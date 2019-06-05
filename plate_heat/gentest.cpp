/*============================================================================
 * Name        : gentest.cpp
 * Author      : Graeme Bragg
 * Version     : 0.1
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

float minChgF = 0.01;

struct fixedNode {
    uint32_t x;
    uint32_t y;
    float t;

};

int main(int argc, const char * argv[]) {
    uint32_t yMax, xMax, nodeCount;
    uint32_t x, y;
    std::string minChg;

    std::string gType;
    std::string gTypeFile;
    std::string gIDstr;
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
    args::ValueFlag<float> mCh(optional, "float", "Set the minimum change in temperature required to trigger a message", {'m'});
    args::ValueFlag<std::string> typF(optional, "filename", "Filename of the Type file, default=plate_heat_type.xml", {'t'});


    try
    {
        parser.ParseCLI(argc, argv);
        } catch (args::Help) {
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
    if(dIn) {
          xMax = yMax = args::get(dIn);
    } else {
         xMax = (xIn)? args::get(xIn) : XDEFAULT;
         yMax = (yIn)? args::get(yIn) : YDEFAULT;
    }

    //Set the minChg
    if(mCh) {
        minChg = args::get(mCh);
    } else {
        minChg = std::to_string(minChgF);
    }

    //Use an alternative typefile
    if(typF) {
        std::string tmpPath = args::get(typF);
        std::ifstream f(tmpPath.c_str());
        if(f.good())
        {
            gTypeFile = tmpPath;
        }
    }


    nodeCount = xMax * yMax;

    std::vector<fixedNode> fNodes = { {0, 0, 100.0},
                                    {(xMax - 1), (yMax - 1), -100.0}
                                  };

    

    //Set the app name
    std::ostringstream ssID;
    ssID << "plate_" << xMax << "x" << yMax;
    gIDstr = ssID.str();

    //Open the files we need
    gFile.open(gIDstr+".xml");        //The generated Graph

    tFile.open(gTypeFile);    //Type file

    eFile.open(gIDstr+"_edges.xml", std::fstream::in | std::fstream::out | std::fstream::trunc); //Scratch file for the edges

    gFile.precision(2);    //Set float precision to 2D.P.

    //Write XML Preamble.
    gFile << "<?xml version=\"1.0\"?>" << std::endl;
    std::cout << "<?xml version=\"1.0\"?>" << std::endl;
    gFile << "<Graphs xmlns=\"https://poets-project.org/schemas/virtual-graph-schema-v2\">" << std::endl;
    std::cout << "<Graphs xmlns=\"https://poets-project.org/schemas/virtual-graph-schema-v2\">" << std::endl;


    unsigned hcMaxScale, hbIdxScale;
    if(nodeCount<10000) {
        std::cout << "\t<10000" << std::endl;
        hbIdxScale = 1000;
        hcMaxScale = 10;
    } else if(nodeCount<700000) {
        std::cout << "\t<700000" << std::endl;
        hbIdxScale = 1000;
        hcMaxScale = 100;
    } else if (nodeCount<1000000) {
        std::cout << "\t<1000000" << std::endl;
        hbIdxScale = 10000; 
        hcMaxScale = 1000;
    } else {
        std::cout << "\t>1000000" << std::endl;
        hbIdxScale = 100000;
        hcMaxScale = 10000;
    }

    //Copy graph type into graph instance.
    std::cout << "<GraphType id=\"" << gType << "\" >" << std::endl;
    while(std::getline(tFile, line))
    {
        std::size_t XSIZE_pos, YSIZE_pos, NODE_pos, HCMAX_pos, HBIDX_pos;

        std::string XSIZE_str ("XSIZE_DEF");
        std::string YSIZE_str ("YSIZE_DEF");
        std::string NODE_str ("NODE_DEF");
        std::string HCMAX_str ("HCMAX_DEF");
        std::string HBIDX_str ("HBIDX_DEF");

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
    for(x=0; x<xMax; x++)
    {
       for(y=0; y<yMax; y++)
       {
            bool isFixed = false;            

            std::string temp;

            for ( auto f : fNodes )    //Work out if this is a fixed node.
            {
                if((f.x == x) && (f.y == y)) {
                    isFixed = true;
                    temp = std::to_string(f.t);
                    break;
                }
            }

            if(isFixed)
            {
                gFile << "      <DevI id=\"c_" << x << "_" << y << "\" type=\"fixedNode\">";
                gFile << "<P>\"t\": " << temp << ", \"x\": " << x << ", \"y\": " << y;
                gFile << "</P></DevI>" << std::endl;

            } else {
                gFile << "      <DevI id=\"c_" << x << "_" << y << "\" type=\"cell\">";
                gFile << "<P>\"x\": " << x << ", \"y\": " << y << ", \"minChg\": " << minChg << "</P></DevI>";
                gFile << std::endl;

                if(y < yMax-1) //North connection
                {
                    eFile << "      <EdgeI path=\"c_" << x << "_" << y << ":north_in-";
                    eFile << "c_" << x << "_" << (y + 1) << ":out\"/>";
                    eFile << std::endl;
                }

                if(x < xMax-1) //East connection
                {
                    eFile << "      <EdgeI path=\"c_" << x << "_" << y << ":east_in-";
                    eFile << "c_" << (x + 1) << "_" << y << ":out\"/>";
                    eFile << std::endl;
                }

                if(y > 0)   //South connection
                {
                    eFile << "      <EdgeI path=\"c_" << x << "_" << y << ":south_in-";
                    eFile << "c_" << x << "_" << (y - 1) << ":out\"/>";
                    eFile << std::endl;
                }

                if(x > 0)   //West connection
                {
                    eFile << "      <EdgeI path=\"c_" << x << "_" << y << ":west_in-";
                    eFile << "c_" << (x - 1) << "_" << y << ":out\"/>";
                    eFile << std::endl;
                }
            }

            //Super Done connection
            //eFile << "      <EdgeI path=\"c_" << x << "_" << y << ":done-:done";
            //eFile << "\" />" << std::endl;

            //Super Finished connection
            eFile << "      <EdgeI path=\":finished-c_" << x << "_" << y;
            eFile << ":finished\"/>" << std::endl;
            eFile << std::endl;
       }
    }

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

    remove((gIDstr+"_edges.xml").c_str());    //Clean up the edges file


    //Write all of the graph-closing foo.
    std::cout << std::endl << "    </EdgeInstances>" << std::endl;
    gFile << "  </GraphInstance>" << std::endl;
    std::cout << "  </GraphInstance>" << std::endl;
    gFile << "</Graphs>" << std::endl;
    std::cout << "</Graphs>" << std::endl;

    gFile.close();
    return 0;
}
