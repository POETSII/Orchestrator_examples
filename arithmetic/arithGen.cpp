/*============================================================================
 * Name        : arithGen.cpp
 * Author      : Graeme Bragg
 * Version     : 0.1.00
 * Description : Generates an example arithmetic problem solver for POETS.
 *                 Tested with C++17 (-std=c++17).
 *                 Requires the type file (<app>_type.xml) in the same directory.
 *============================================================================
 */
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <stdio.h>
#include <math.h>
#include <ctime>
#include <random>
#include <limits>
#include <cstdint>
#include "args/args.hxx"

class edge
{
    public:
    uint32_t from;
    uint32_t to;
    
    edge(uint32_t, uint32_t);
    
    void writeEdge(std::ofstream&);
};

class node
{
    public:
    uint32_t idx;       // The node's index (a function of X and Y)
    uint32_t iSeed;     // The Integer seed for the device
    float fSeed;        // The float seed for the device
    uint32_t nMax;      // The number of connections that the node has
    
    std::vector<edge> inEdges;      // Vector of input edges
    
    node(uint32_t, uint32_t, float, uint32_t);
    
    void writeNode(std::ofstream&);
};

edge::edge(uint32_t cFr, uint32_t cTo)
{
    from = cFr;
    to = cTo;
}

// Write the edge values to the specified file.
void edge::writeEdge(std::ofstream& eFile)
{
    eFile << "\t\t<EdgeI path=\"N" << to << ":in-N" << from << ":out\"/>\n";
}

node::node(uint32_t cId, uint32_t cIs, float cFs, uint32_t cNm)
{
    idx = cId;
    iSeed = cIs;
    fSeed = cFs;
    nMax = cNm;
}

// Write the entry for the node to the specified file.
void node::writeNode(std::ofstream& nFile)
{
    nFile << "\t\t<DevI id=\"N" << idx << "\" type=\"node\" ";
    nFile << "P=\"{" << idx << "," << iSeed << "," << fSeed << "," << nMax;
    nFile << "}\"" << "/>\n";
}


inline uint32_t xy2i(uint32_t x, uint32_t y, uint32_t dimension)
{
    return x + (y * dimension);
}

/* Populate the specified node vector with a 2D grid of dimension <dimSz>
 */
void gridGen(std::vector<node>& nodes, uint32_t& dimSz)
{
    // Random float machinery
    std::default_random_engine fEngine;
    std::uniform_real_distribution<float> fRand(0, 1); // rage 0 - 1
    
    // Random int machinery
    std::default_random_engine iEngine;
    std::uniform_int_distribution<uint32_t> iRand(0, UINT32_MAX);
    
    for(uint32_t y = 0; y < dimSz; y++)
    {
        for(uint32_t x = 0; x < dimSz; x++)
        {
            // Add a node
            
            uint32_t idx = xy2i(x,y,dimSz);
            uint32_t iSeed = iRand(iEngine);
            float fSeed = fRand(fEngine);
            
            node nd = node(idx,iSeed,fSeed,0);
            
            // EAST connection
            if(x<(dimSz-1))
            {
                nd.inEdges.push_back(edge(xy2i((x+1),y,dimSz),idx));
                nd.nMax++;
            }
            
            // NORTH connection
            if(y<(dimSz-1))
            {
                nd.inEdges.push_back(edge(xy2i(x,(y+1),dimSz),idx));
                nd.nMax++;
            }
            
            // WEST connection
            if(x>0)
            {
                nd.inEdges.push_back(edge(xy2i((x-1),y,dimSz),idx));
                nd.nMax++;
            }
            
            // SOUTH connection
            if(y>0)
            {
                nd.inEdges.push_back(edge(xy2i(x,(y-1),dimSz),idx));
                nd.nMax++;
            }
            
            // Add the node to the list
            nodes.push_back(nd);
        }
    }
}



void generate(uint32_t dimSz, uint32_t iterMax, std::string oFname, bool constrain,
                bool eightbox)
{
    std::cout << "Generating Dummy Arithmetic XML:" << std::endl;
    std::cout << "\t Dimsension Size=\t\t" << dimSz << std::endl;
    
    std::vector<node> nodes;
    
    gridGen(nodes, dimSz);
    
    std::ofstream oFile, cFile;
    oFile.open("./"+oFname+".xml");
    
    // Write the preamble
    oFile << "<?xml version=\"1.0\"?>\n";
    oFile << "<!-- Generated using the POETS Orchestrator Dummy Arit proof of ";
    oFile << "concept.\n-->\n";
    oFile << "<Graphs xmlns=\"\" appname=\"Arith\">\n";
    
    
    
    
    // Copy the type.
    std::ifstream tFile;
    tFile.open("./arith_type.xml");    //Type file
    std::string line;
    while(std::getline(tFile, line))
    {
        oFile << line << std::endl;
    }
    tFile.close();
    
    
    /* Now to write out the actual graph instance. This needs to iterate over
     * timesteps twice: once for the device instances and once for the edge
     * instances.
     */
    oFile << "  <GraphInstance id=\"calc\" graphTypeId=\"calc\" ";
    oFile << "P=\"{" << (dimSz*dimSz) << "," << iterMax << "}\">\n";
    
    oFile << "\t<DeviceInstances>\n";
    
    // Iterate over the nodes and write them out
    for(std::vector<node>::iterator nodeIt = nodes.begin();
        nodeIt != nodes.end(); nodeIt++)
    {
        (*nodeIt).writeNode(oFile);
    }
    
    oFile << "\t</DeviceInstances>\n";
    
    oFile << "\t<EdgeInstances>\n";
    // For each timestep, write out the edge instances.
    for(std::vector<node>::iterator nodeIt = nodes.begin();
            nodeIt != nodes.end(); nodeIt++)
    {
        // Iterate over the node's edges and write them out
        for(std::vector<edge>::iterator edgeIt = (*nodeIt).inEdges.begin();
            edgeIt != (*nodeIt).inEdges.end(); edgeIt++)
        {
            (*edgeIt).writeEdge(oFile);
        }
    }
    oFile << "\t</EdgeInstances>\n";
    
    
    oFile << "  </GraphInstance>\n";    // Close out the GraphInstance
    oFile << "</Graphs>\n";             // Close out the XML
    
    oFile.close();
    
    // Now generate a call file
    cFile.open(oFname+".poets");
    if(eightbox)
    {
        cFile << "load /engine = ";
        cFile << "\"../Tests/StaticResources/Dialect3/Valid/8_box.uif\"\n";
    }
    cFile << "load /app = +\"" << oFname << ".xml\"\n";
    cFile << "tlink /app = *\n";
    cFile << "place /spread = *\n";
    cFile << "compose /app = *\n";
    cFile << "deploy /app = *\n";
    cFile << "initialise /app = *\n";
    cFile << "run /app = *\n";
    std::cout << "Call file" << std::endl;
    cFile.close();
}


int main(int argc, const char * argv[])
{   
    uint32_t dimSz = 3;     // 3x3 grid
    uint32_t iterMax = 10000;   // 10k iterations
    
    std::string oFname;
    
    bool constrain = false;
    bool eightbox = false;
    
    //Handle arguments for generation
    args::ArgumentParser parser("Application: plate_heat generator, POETS Project\n");
    parser.LongSeparator("=");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::Group optional(parser, "All of these arguments are optional:", args::Group::Validators::DontCare);
    args::Group oneof(parser, "At most one of these options:", args::Group::Validators::AtMostOne);
    
    
    args::ValueFlag<int> ncIn(optional, "int", "Set node count default=3", {'n', "dimSz"});
    args::ValueFlag<int> imIn(optional, "int", "Set iteration count default=10000", {'i', "iterMax"});
    
    args::ValueFlag<std::string> oAppend(optional, "string", "String to append to the generated filename before .xml, default=\"\"", {'o'});
    args::ValueFlag<std::string> oReplace(optional, "string", "String to use instead of the generated filename, default=\"\"", {'O'});
    
    args::Flag cnIn(optional, "Constrain", "Constrain the number of devices per thread to distribute the problem over more of the system. Default = false", {'c'});
    args::Flag ebIn(optional, "Eightbox", "Flag that the generated call file should be for an eight-box system. Default = false", {'8'});
   
    
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
    
    // Set the dimSz
    if(ncIn) 
    {
        dimSz = args::get(ncIn);
    }
    
    // Set the iteration count
    if(imIn) 
    {
        iterMax = args::get(imIn);
    }
    
    // Application execution constraints
    if(cnIn)            constrain = true;
    if(ebIn)            eightbox = true;
    
    // Sort out the output filename
    oFname = "calc";
    if(oReplace)
    {
        oFname = args::get(oReplace);
    }
    else if(oAppend)
    {
        oFname += "_";
        oFname += args::get(oAppend);
    }
    
    generate(dimSz,iterMax,oFname,constrain,eightbox);
    return 0;
}
