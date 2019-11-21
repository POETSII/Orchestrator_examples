/*============================================================================
 * Name        : gentest.cpp
 * Author      : Graeme Bragg
 * Version     : 0.3.56
 * Description : Generates an example heated plate that is solved with a
 *                 packet storm.
 *                 Tested with C++17 (-std=c++17).
 *                 Requires the type file (<app>_type.xml) in the same directory.
 *                 An arbitrary number of fixed nodes can be specified in the fNodes
 *                 vector.
 *                 To minimise memory usage for very large graphs, the generator
 *                 writes the edges to a separate file (<app>_XSIZExYSIZE_edges.xml)
 *                 before iterating back through to add them to the application XML.
 *
 *                 Currently:
 *                   * dimensions are specified as commandline arguments, with defaults
 *                     in YDEFAULT and XDEFAULT.
 *                   * Application name is set to plate_XxY and the type file is
 *                     plate_heat_type.xml.
 *                   * Two fixed nodes are specified at 0,0 and XSIZE-1,YSIZE-1
 *============================================================================
 */

#define XDEFAULT 3
#define YDEFAULT 3

#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include "args/args.hxx"

uint32_t yMax, xMax, nodeCount;

struct fixedNode {
    uint32_t x;
    uint32_t y;
    float t;

};


void writeDev(uint32_t x, uint32_t y, std::vector<fixedNode>& fNodes,
                std::string& minChg, std::ofstream& gFile, std::fstream& eFile,
                bool dummy)
{
    bool isFixed = false;
    std::string temp;
    
    for (auto f : fNodes)//Work out if this is a fixed node.
    {
        if((f.x == x) && (f.y == y)) {
            isFixed = true;
            temp = std::to_string(f.t);
            break;
        }
    }

    if(isFixed)
    {
        gFile << "      <DevI id=\"c_" << x << "_" << y; 
        gFile << "\" type=\"fixedNode\">";
        gFile << "<P>\"t\": " << temp << ", \"x\": " << x; 
        gFile << ", \"y\": " << y;
        gFile << "</P></DevI>" << std::endl;
        
        if(dummy && (x < (xMax-1) || y < (yMax-1)))
        {
            // write a dummy padding  device 
            gFile << "      <DevI id=\"dummy_" << x << "_" << y; 
            gFile << "\" type=\"cell\">";
            gFile << "<P>\"dummy\": 1" << "</P></DevI>";
            gFile << std::endl;
        }
    } else {
        gFile << "      <DevI id=\"c_" << x << "_" << y;
        gFile << "\" type=\"cell\">";
        gFile << "<P>\"x\": " << x << ", \"y\": " << y;
        if(minChg != "")
        {
            gFile << ", \"minChg\": " << minChg;
        }
        gFile << "</P></DevI>";
        gFile << std::endl;

        if(y < yMax-1) //North connection
        {
            eFile << "      <EdgeI path=\"c_";
            eFile << x << "_" << y << ":north_in-";
            eFile << "c_" << x << "_" << (y + 1);
            eFile << ":out\"/>";
            eFile << std::endl;
        }

        if(x < xMax-1) //East connection
        {
            eFile << "      <EdgeI path=\"c_";
            eFile << x << "_" << y << ":east_in-";
            eFile << "c_" << (x + 1);
            eFile << "_" << y << ":out\"/>";
            eFile << std::endl;
        }

        if(y > 0)   //South connection
        {
            eFile << "      <EdgeI path=\"c_";
            eFile << x << "_" << y << ":south_in-";
            eFile << "c_" << x << "_" << (y - 1);
            eFile << ":out\"/>";
            eFile << std::endl;
        }

        if(x > 0)   //West connection
        {
            eFile << "      <EdgeI path=\"c_";
            eFile << x << "_" << y << ":west_in-";
            eFile << "c_" << (x - 1);
            eFile << "_" << y << ":out\"/>";
            eFile << std::endl;
        }
        
        //eFile << "      <EdgeI path=\":instr-c_";
        //eFile << x << "_" << y;
        //eFile << ":instr\"/>" << std::endl;
        //eFile << std::endl;
    }

    // Finished 
    eFile << "      <EdgeI path=\":finished-c_";
    eFile << x << "_" << y;
    eFile << ":finished\"/>" << std::endl;
    
    if(!isFixed)
    {
        // Heartbeat Hack
        eFile << "      <EdgeI path=\"c_" << x << "_" << y << ":heart_in-";
        eFile << "c_" << x << "_" << y << ":heart_out\"/>" << std::endl;
    }
    eFile << std::endl;
}


int main(int argc, const char * argv[]) {
    uint32_t squares, squaresDimension;
    uint32_t x, y, xStart, yStart, threadMaxCount;
    std::vector<fixedNode> fNodes;
    
    std::string minChg;
    float fixTmp;
    int fixPat;
    
    int hbOver, idleOver;

    std::string gType;
    std::string gTypeFile;
    std::string gIDstr;
    std::string gAppend;
    std::string gInstance;

    std::ofstream gFile;
    std::ifstream tFile;
    std::fstream eFile;
    std::string line;

    //set the type name
    gType = std::string("plate_heat");
    gTypeFile = gType+"_type.xml";


    //Handle arguments for X and Y
    args::ArgumentParser parser("Application: plate_heat generator, POETS Project\n");
    parser.LongSeparator("=");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});

    args::Group optional(parser, "All of these arguments are optional:", args::Group::Validators::DontCare);
    args::ValueFlag<int> xIn(optional, "int", "Set X Dimension, default=3", {'x'});
    args::ValueFlag<int> yIn(optional, "int", "Set Y Dimension, default=3", {'y'});
    args::ValueFlag<int> dIn(optional, "int", "Set Both Dimensions, default=3", {'d'});
    args::ValueFlag<float> mCh(optional, "float", "Set the minimum change in temperature required to trigger a message. Default = 0.5", {'m'});
    args::ValueFlag<float> tFx(optional, "float", "Set the temperature of the fixed nodes. Default = 100", {'t'});
    args::ValueFlag<int> fNo(optional, "int", "Fixed-node pattern. (0) 2 opposite corners, (1) 4 opposite corners. Default=0", {'f'});
    args::ValueFlag<std::string> typF(optional, "filename", "Filename of the Type file, default=plate_heat_type.xml", {'x', "type"});
    args::ValueFlag<std::string> oAppend(optional, "string", "String to append to the generated filename before .xml, default=\"\"", {'o'});
    args::ValueFlag<int> sq(optional, "int", "Squares: Set whether devices are generated linearly (0) or in blocks (1 = thread-level, 2 = box-level), default = 0", {'s'});
    args::ValueFlag<int> sd(optional, "int", "Square Dimension - used to set the x & y dimension of the square size, default = 32 (e.g. 1024 devices)", {'u', "sd"});
    args::ValueFlag<int> tMC(optional, "int", "Set the maximum number of threads (used for Supervisor Instrumentation Array sizing). Default = 49152", {"ThreadMaxCount"});
    args::ValueFlag<int> idleIn(optional, "int", "Set the OnIdle count required to trigger a HB, default >=1000 (scales with problem size)", {'i'});
    args::ValueFlag<int> hbIn(optional, "int", "Set the HB count required to trigger a finish message, default = 10", {'z', "hb"});


    try
    {
        parser.ParseCLI(argc, argv);
        } catch (args::Help&) {
            std::cout << parser;
            return -1;
        } catch (const args::ParseError& e) {
            std::cerr << e.what() << std::endl;
            std::cerr << parser;
            return -1;
        } catch (const args::ValidationError& e) {
            std::cerr << e.what() << std::endl;
            std::cerr << parser;
            return -1;
    }

    //Set Dimensions
    if(dIn) 
    {
          xMax = yMax = args::get(dIn);
    } else {
         xMax = (xIn)? args::get(xIn) : XDEFAULT;
         yMax = (yIn)? args::get(yIn) : YDEFAULT;
    }

    //Set the minChg
    if(mCh) 
    {
        minChg = std::to_string(args::get(mCh));
    } else {
        minChg = "";
    }
    
    //Set the fixed temperature
    if(tFx) 
    {
        fixTmp = args::get(tFx);
    } else {
        fixTmp = 100.0;
    }
    
    //Set the fixed node pattern
    if(fNo) 
    {
        fixPat = args::get(fNo);
    } else {
        fixPat = 0;
    }

    //Use an alternative typefile
    if(typF)
    {
        std::string tmpPath = args::get(typF);
        std::ifstream f(tmpPath.c_str());
        if(f.good())
        {
            gTypeFile = tmpPath;
        }
    }
    
    //String to append to output filename
    if(oAppend)
    {
        gAppend = args::get(oAppend);
    } else {
        gAppend = "";
    }
    
    //Define the crude mapping
    if(sq)
    {
        squares = args::get(sq);
    } else {
        squares = 0;
    }
    
    if(squares==2 && (xMax != yMax))
    {
      std::cout << "Error: s2 can only be used when x=y" << std::endl;
      return 1;
    }
    
    if(squares==2 && (xMax > 6144))
    {
      std::cout << "Error: s2 can only be used when x<=6144" << std::endl;
      return 1;
    }
    
    if(sd)
    {
        squaresDimension = args::get(sd);
    } else {
        squaresDimension = 32;
    }
    
    //Set the thread Max Count
    if(tMC) 
    {
        threadMaxCount = args::get(tMC);
    } else {
        threadMaxCount = 49152;
    }
    
    
    // Setup heartbeat override
    if(idleIn)
    {
        idleOver = args::get(idleIn);
    } else {
        idleOver = 0;
    }
    
    if(hbIn)
    {
        hbOver = args::get(hbIn);
    } else {
        hbOver = 0;
    }

    nodeCount = xMax * yMax;

    switch(fixPat){
      case 1:   fNodes = { {0, 0, fixTmp}, 
                           {0, (yMax - 1), -fixTmp},
                           {(xMax - 1), 0, -fixTmp},
                           {(xMax - 1), (yMax - 1), fixTmp}
                         };
                break;
      case 0:
      default:  fNodes = {{0, 0, fixTmp}, {(xMax - 1), (yMax - 1), -fixTmp}};
    }

    

    //Set the app name
    std::ostringstream ssID;
    ssID << "plate_" << xMax << "x" << yMax;
    gIDstr = ssID.str();

    //Open the files we need
    gFile.open(gIDstr+gAppend+".xml");        //The generated Graph

    tFile.open(gTypeFile);    //Type file

    eFile.open(gIDstr+gAppend+"_edges.xml", std::fstream::in | std::fstream::out | std::fstream::trunc); //Scratch file for the edges

    gFile.precision(2);    //Set float precision to 2D.P.

    //Write XML Preamble.
    gFile << "<?xml version=\"1.0\"?>" << std::endl;
    std::cout << "<?xml version=\"1.0\"?>" << std::endl;
    gFile << "<Graphs xmlns=\"https://poets-project.org/schemas/virtual-graph-schema-v2\">" << std::endl;
    std::cout << "<Graphs xmlns=\"https://poets-project.org/schemas/virtual-graph-schema-v2\">" << std::endl;


    unsigned hcMaxScale, hbIdxScale;
    if(nodeCount<50000) {
        std::cout << "\t<1000" << std::endl;
        hbIdxScale = 1000;
        hcMaxScale = 10;
    } else if(nodeCount<100000) {
        std::cout << "\t<100000" << std::endl;
        hbIdxScale = 2000;
        hcMaxScale = 10;
    } else if(nodeCount<100000) {
        std::cout << "\t<100000" << std::endl;
        hbIdxScale = 5000;
        hcMaxScale = 10;
    } else if(nodeCount<700000) {
        std::cout << "\t<700000" << std::endl;
        hbIdxScale = 5000;
        hcMaxScale = 10;
    } else if (nodeCount<1000000) {
        std::cout << "\t<1000000" << std::endl;
        hbIdxScale = 7000; 
        hcMaxScale = 10;
    } else {
        std::cout << "\t>1000000" << std::endl;
        hbIdxScale = 2000;
        hcMaxScale = 10;
    }
    
    if(hbOver > 0)
    {
        hcMaxScale = hbOver;
    }
    
    if(idleOver > 0)
    {
        hbIdxScale = idleOver;
    }

    //Copy graph type into graph instance.
    std::cout << "<GraphType id=\"" << gType << "\" >" << std::endl;
    while(std::getline(tFile, line))
    {
        std::size_t XSIZE_pos, YSIZE_pos, NODE_pos, HCMAX_pos, HBIDX_pos, THREAD_pos;

        std::string XSIZE_str ("XSIZE_DEF");
        std::string YSIZE_str ("YSIZE_DEF");
        std::string NODE_str ("NODE_DEF");
        std::string HCMAX_str ("HCMAX_DEF");
        std::string HBIDX_str ("HBIDX_DEF");
        std::string THREAD_str ("THREAD_DEF");

        XSIZE_pos = line.find(XSIZE_str);
        if(XSIZE_pos != std::string::npos){
            // Line contains XSIZE_DEF - replace with x
            line.replace(XSIZE_pos, XSIZE_str.length(), std::to_string(xMax));
        }

        YSIZE_pos = line.find(YSIZE_str);
        if (YSIZE_pos != std::string::npos){
            // Line contains YSIZE_DEF - replace with y
            line.replace(YSIZE_pos, YSIZE_str.length(), std::to_string(yMax));
        }

        NODE_pos = line.find(NODE_str);
        if (NODE_pos != std::string::npos){
            // Line contains NODE_DEF - replace with x*y
            line.replace(NODE_pos, NODE_str.length(), std::to_string(nodeCount));
        }

        HCMAX_pos = line.find(HCMAX_str);
        if (HCMAX_pos != std::string::npos){
            // Line contains HCMAX_DEF - replace with 
            line.replace(HCMAX_pos, HCMAX_str.length(), std::to_string(hcMaxScale));
        }

        HBIDX_pos = line.find(HBIDX_str);
        if (HBIDX_pos != std::string::npos){
            // Line contains HBIDX_DEF - replace with 
            line.replace(HBIDX_pos, HBIDX_str.length(), std::to_string(hbIdxScale));
        }
        
        THREAD_pos = line.find(THREAD_str);
        if (THREAD_pos != std::string::npos){
            // Line contains THREAD_DEF - replace with 
            line.replace(THREAD_pos, THREAD_str.length(), std::to_string(threadMaxCount));
        }

        gFile << line << std::endl;        //TODO: modify to only read graph instance, error handling, etc.
    }
    std::cout << std::endl << "</GraphType>" << std::endl;
    gFile << std::endl;

    //Form the GraphInstance member
    std::ostringstream ssInst;
    ssInst << "  <GraphInstance id=\"" << gIDstr << "\" graphTypeId=\"" << gType << "\">" << std::endl;
    ssInst << "<Properties>\"xSize\": " << xMax << ", \"ySize\": " << yMax << ", \"nodeCount\": " << nodeCount << "</Properties>";
    gInstance = ssInst.str();

    //Write all of the instance preamble
    std::cout << gInstance << std::endl;
    gFile << gInstance << std::endl;
    gFile << "    <DeviceInstances>" << std::endl;
    std::cout << "    <DeviceInstances>" << std::endl;
    eFile << "    <EdgeInstances>" << std::endl;

    //Loop through all devices and write device instance + edge instance.
    //for(x=0; x<xMax; x++)
    if(squares == 2)
    {
      /* Generate devices in a specific order so that they reduce the number of 
       * connections leaving each thread/core/mailbox/board/box when used with
       * the naive bucket-filling placement.
       *
       * This can generate a LOT of padding/dummy devices 
       *
       * This is nasty due to goto, but hey, this generator is quick & dirty,
       * so fits the ethos...
       */   
      unsigned xCoreStart = 0, yCoreStart = 0;
      
       
      //Generate a system-full
      for(unsigned xBox = 0; xBox < 2; xBox++)
      {
        for(unsigned yBox = 0; yBox < 3; yBox++)
        {
      
          //Generate a box-full
          for(unsigned xBoard = 0; xBoard < 3; xBoard++)
          {
            for(unsigned yBoard = 0; yBoard < 2; yBoard++)
            {
              
              //Generate a board-full
              for(unsigned xMbox = 0; xMbox < 4; xMbox++)
              {
                for(unsigned yMbox = 0; yMbox < 4; yMbox++)
                {
                  std::cout << "xMbox:" << xMbox << "\tyMbox:" << yMbox << std::endl;
                  
                  
                  // Generate a Mailbox full
                  for(unsigned yCore = 0; yCore < 2; yCore++)
                  {
                    xCoreStart = 256*xMbox + 1024*xBoard + 3072*xBox;
                    for(unsigned xCore = 0; xCore < 2; xCore++)
                    {
                      std::cout << "\txCore:" << xCore << "\tyCore:" << yCore; 
                      std::cout << "\t\txCoreStart:" << xCoreStart;
                      std::cout << "\tyCoreStart:" << yCoreStart << std::endl;
                      
                      
                      
                      yCoreStart = 128*yCore + 256*yMbox + 1024*yBoard + 2048*yBox;
                      if(yCoreStart >= yMax && xCoreStart >= xMax)
                      {
                          goto bailout;
                      } 
                      else if(yCoreStart < yMax && xCoreStart < xMax) // Don't generate a core that is just padding
                      {
                        
                        // Generate a core-full, pad everything
                        for(unsigned xThread = 0; xThread < 4; xThread++,xCoreStart+=squaresDimension)
                        { 
                          yStart = yCoreStart;
                          std::cout << "\t\txThread:" << xThread << "\txStart:" << xCoreStart << std::endl;
                          for(unsigned yThread = 0; yThread < 4; yThread++,yStart+=squaresDimension)
                          {
                            std::cout << "\t\t\tyThread:" << yThread << "\tyStart:" << yStart << std::endl;
                        
                            // Generate a thread-full, pad everything 
                            for(x=xCoreStart; x<(xCoreStart+squaresDimension); x++)
                            {
                              for(y=yStart; y<(yStart+squaresDimension); y++)
                              {                   
                                if(x >= xMax || y >= yMax)
                                {
                                  // write a dummy padding  device 
                                  gFile << "      <DevI id=\"dummy_" << x << "_" << y;
                                  gFile << "\" type=\"cell\"/>";
                                  gFile << std::endl;
                                }
                                else
                                {
                                  writeDev(x, y, fNodes, minChg, gFile, eFile, true);
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
      

    } else if(squares == 1) {
        /* Generate devices in thread-size chunks (with dummy devices).
         */ 
        for(xStart = 0; xStart < xMax; xStart+=squaresDimension)
        {    
            for(yStart = 0; yStart < yMax; yStart+=squaresDimension)
            {
                for(x=xStart; x<(xStart+squaresDimension); x++)
                {
                    for(y=yStart; y<(yStart+squaresDimension); y++)
                    {
                        if(x >= xMax && y >= yMax)
                        {
                            
                        }                    
                        else if(x >= xMax || y >= yMax)
                        {
                            // write a dummy padding  device 
                            gFile << "      <DevI id=\"dummy_" << x << "_" << y;
                            gFile << "\" type=\"cell\"/>";
                            gFile << std::endl;
                        }
                        else
                        {
                            writeDev(x, y, fNodes, minChg, gFile, eFile, true);
                        }
                    }
                }
            }
        }
    } else {
        /* Generate devices in coordinate order */
        for(x=0; x<xMax; x++)
        {
            for(y=0; y<yMax; y++)
            {
                writeDev(x, y, fNodes, minChg, gFile, eFile, false);
            }
        }
    }
    
bailout:
    gFile << "    </DeviceInstances>" << std::endl;
    std::cout << std::endl << "    </DeviceInstances>" << std::endl;
    eFile << "    </EdgeInstances>" << std::endl;

    //Copy edges into graph instance.
    std::cout << "    <EdgeInstances>" << std::endl;
    eFile.seekg(0);
    while(std::getline(eFile, line))
    {
        gFile << line << std::endl;
    }
    eFile.close();

    remove((gIDstr+gAppend+"_edges.xml").c_str());    //Clean up the edges file


    //Write all of the graph-closing foo.
    std::cout << std::endl << "    </EdgeInstances>" << std::endl;
    gFile << "  </GraphInstance>" << std::endl;
    std::cout << "  </GraphInstance>" << std::endl;
    gFile << "</Graphs>" << std::endl;
    std::cout << "</Graphs>" << std::endl;

    gFile.close();
    return 0;
}
