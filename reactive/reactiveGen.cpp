/*============================================================================
 * Name        : reactiveGen.cpp
 * Author      : Graeme Bragg
 * Version     : 0.4.00
 * Description : Generates an example reactive circuit simulation for POETS.
 *                 Tested with C++17 (-std=c++17).
 *                 Requires the type file (<app>_type.xml) in the same directory.
 *                 Includes two different circuit types for simulation:
 *                  * A basic single-node unforced RC circuit for validation
 *                  * A configurable length RC transmission line.
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
#include "args/args.hxx"

typedef enum EdgeType_T {conductance = 0, reactance = 1, isrc = 2, past} edget_t;
typedef enum IType_T {standard = 0, inverted, twoS, invertedtwoS} itype_t;

class edge
{
    public:
    uint32_t from;
    uint32_t to;
    
    edget_t type;
    float arg;
    
    edge(uint32_t, uint32_t, edget_t, float);
    
    void writeEdge(uint32_t, std::ofstream&);
};

class node
{
    public:
    uint32_t id;
    float iZx;
    float Rgnd;
    uint32_t Ignd;
    itype_t Itype;
    float Vxt;
    float Vxt1;
    
    std::vector<edge> inEdges;
    
    node(uint32_t, float, float, uint32_t, itype_t);
    
    void writeNode(uint32_t, float, std::ofstream&);
    
    static float I(float, float, float, float, float, float, float, float, uint32_t);
};

struct params
{
    float R1;
    float R2;
    float C1;
    float C2;
    float V1;   // An initial value for a node's V
    float V2;   // An initial value for a node's V
};

edge::edge(uint32_t cFr, uint32_t cTo, edget_t cTy, float cAr)
{
    from = cFr;
    to = cTo;
    type = cTy;
    arg = cAr;
}

// Write the edge values to the specified file.
void edge::writeEdge(uint32_t stepIdx, std::ofstream& eFile)
{
    if(type == past)
    {
        // Format is N<from>_<stepIdx+1>:past-N<from>_<stepIdx>:out
        eFile << std::setprecision(std::numeric_limits<float>::digits10 + 1);
        eFile << "\t\t<EdgeI path=\"N" << from << "_" << stepIdx;
        eFile << ":past-N" << from << "_" << (stepIdx - 1) << ":out\"/>\n";
    }
    else
    {
        // Format is N<to>_<stepIdx>:in-N<from>_<stepIdx>:out
        eFile << std::setprecision(std::numeric_limits<float>::digits10 + 1);
        eFile << "\t\t<EdgeI path=\"N" << to << "_" << stepIdx;
        eFile << ":in-N" << from << "_" << stepIdx << ":out\" ";
        eFile << "P=\"{" << type << "," << arg << "}\"/>\n";
    }
}

typedef enum genType_t {decay, transmission, grid} genType_t;

node::node(uint32_t cId, float cIz, float cRg, uint32_t cIg, 
            IType_T cIt = standard)
{
    id = cId;
    iZx = cIz;
    Rgnd = cRg;
    Ignd = cIg;
    Itype = cIt;
    Vxt = 0.0f;
    Vxt1 = 0.0f;
}

// Write the entry for the node in the specified timestep to the specified file.
void node::writeNode(uint32_t stepIdx, float t, std::ofstream& nFile)
{
    float IgndVal = 0.0f;
    
    switch(Itype)
    {
        case twoS:  
                    for(uint32_t i = Ignd; i > 0; i--)
                    {
                        /* Hard-coded SPICE pulse with the following parameters:
                         *      Ioff =      0
                         *      Ion =       1
                         *      Tdelay =    0.2
                         *      Tr =        0.1
                         *      Tf =        0.1
                         *      Ton =       0.9
                         *      Tperiod =   2
                         *      Ncycles =   0
                         */ 
                        IgndVal -= I(t, 0, 1, 0.2, 0.1, 0.1, 0.9, 2, 0);
                    }
                    break;
        case invertedtwoS:  
                    for(uint32_t i = Ignd; i > 0; i--)
                    {
                        /* Hard-coded SPICE pulse with the following parameters:
                         *      Ioff =      0
                         *      Ion =       -1
                         *      Tdelay =    1.2
                         *      Tr =        0.1
                         *      Tf =        0.1
                         *      Ton =       0.9
                         *      Tperiod =   2
                         *      Ncycles =   0
                         */ 
                        IgndVal -= I(t, 0, -1, 1.2, 0.1, 0.1, 0.9, 2, 0);
                    }
                    break;
        case inverted:  
                    for(uint32_t i = Ignd; i > 0; i--)
                    {
                        /* Hard-coded SPICE pulse with the following parameters:
                         *      Ioff =      0
                         *      Ion =       -1
                         *      Tdelay =    0.2
                         *      Tr =        0.1
                         *      Tf =        0.1
                         *      Ton =       1
                         *      Tperiod =   1
                         *      Ncycles =   0
                         */ 
                        IgndVal -= I(t, 0, -1, 1.2, 0.1, 0.1, 1, 1.3, 0);
                    }
                    break;
        case standard:
                    for(uint32_t i = Ignd; i > 0; i--)
                    {
                        /* Hard-coded SPICE pulse with the following parameters:
                         *      Ioff =      0
                         *      Ion =       1
                         *      Tdelay =    0.2
                         *      Tr =        0.1
                         *      Tf =        0.1
                         *      Ton =       1
                         *      Tperiod =   1
                         *      Ncycles =   0
                         */ 
                        IgndVal -= I(t, 0, 1, 0.2, 0.1, 0.1, 1, 1.3, 0);
                    }
                    break;
        default:    break;
    }
    
    nFile << std::setprecision(std::numeric_limits<float>::digits10 + 1);
    nFile << "\t\t<DevI id=\"N" << id << "_" << stepIdx << "\" type=\"node\" ";
    nFile << "P=\"{" << id << "," << stepIdx << "," << t << "," << iZx;
    nFile << "," << Rgnd << "," << IgndVal << "}\"";
    
    if((Vxt != 0.0f) || (Vxt1 != 0.0f))
    {   // Initial state overide
        nFile << " S=\"{" << Vxt << "," << Vxt1 << "\"}";
    }
    
    nFile << "/>\n";
}


/* Method to generate the value of a time-varying current source at time t.
 * Other parameters have the same definitions as in LTSpice:
 *      Ioff =      Initial value
 *      Ion =       Pulsed value
 *      Tdelay =    Startup delay
 *      Tr =        Rise time
 *      Tf =        Fall time
 *      Ton =       On time
 *      Tperiod =   Time period of the signal
 *      Ncycles =   Number of cycles to run for
 */
float node::I(float t, float Ioff, float Ion, float Tdelay, float Tr, float Tf,  
                float Ton, float Tperiod, uint32_t Ncycles)
{
    float TperiodA = Tr + Ton + Tf;
    if(TperiodA < Tperiod) TperiodA = Tperiod;      // Get the real Tperiod

    if(t < Tdelay)     return Ioff;      // Before the end point

    t -= Tdelay;    // compensate for Tdelay

    if(Ncycles>0)     	// If max cycles is specified,
    {               	// Check to see if we are past endpoint
        uint32_t cycle = (uint32_t)ceil(t/TperiodA);     // Get the actual cycle
        if(cycle > Ncycles) return Ioff;                // Past the end point
    }


    float Ir = Ion - Ioff;

    t = fmod(t, TperiodA);                            // Get time in curr period
    if(t < Tr)      return (((1.0f/Tr)*t)*Ir)+Ioff;   // Rising edge

    t -= Tr;                                          // compensate for Tr
    if(t < Ton)     return Ion;                       // "on" period

    t -= Ton;                                         // compensate for Ton
    if(t < Tf)      return (((-1.0f/Tf)*t)*Ir)+Ion;   // Falling edge

    return Ioff;                                      // "off" period
}

/* Populate the specified node vector with a transmission line <nodeCount> nodes
 * long. This involves two resistor values (R1 and R2) and one capacitor value
 * (C1). Initial voltages and other values in the params are not used.
 *
 *          N0    ┌──────┐   N1  ┌──────┐   N2         ┌──────┐  N<nodeCount-1>
 *  ┌───────┬─────┤  R2  ├───┬───┤  R2  ├───┬── ─ ─ ───┤  R2  ├───┐ 
 *  │       │     └──────┘   │   └──────┘   │          └──────┘   │
 *  │     ┌─┴─┐              │              │                     │
 *   \    │   │              │              │                     │
 * |↓|    │   │R1           ═══ C1         ═══ C1                ═══ C1
 * \      │   │              │              │                     │
 *  │     └─┬─┘              │              │                     │
 *  │       │                │              │                     │
 *  └───────┴────────────────┼──────────────┴── ─ ─ ──────────────┘
 *                          GND
 */
void tlineGen(std::vector<node>& nodes, uint32_t& nodeCount,
                    params p, float tStep)
{
    for(uint32_t i = 0; i < nodeCount; i++)
    {
        if(i==0)
        {   // 1st device, has a current source & R to Gnd, and R to N1.
            // There are no reactive components.
            float iZx = 1.0f/((1.0f/p.R1) + (1.0f/p.R2));
            
            nodes.push_back(node(i,iZx,0.0f,1));
            
            // Add a "conductance" edge 
            nodes.back().inEdges.push_back(edge(1,0,conductance,(1.0f/p.R2)));
            
            
        }
        else if(i == (nodeCount - 1))
        {   // Last device, R to N(i - 1), C to Gnd.
            // One conductances and a reactance to ground.
            float iZx = 1.0f/((1.0f/p.R2) + (p.C1/tStep));
            
            float Rgnd = (p.C1/tStep);
            
            nodes.push_back(node(i,iZx,Rgnd,0));
            
            // Add a "conductance" edge to the left and right
            nodes.back().inEdges.push_back(edge((i-1),i,conductance,(1.0f/p.R2))); 
        }
        else
        {   // "normal" nodes. R to N(i-1), R to N(i+1), C to Gnd.
            // Two conductances and a reactance to ground.
            float iZx = 1.0f/((1.0f/p.R2) + (1.0f/p.R2) + (p.C1/tStep));
            
            float Rgnd = (p.C1/tStep);
            
            nodes.push_back(node(i,iZx,Rgnd,0));
            
            // Add a "conductance" edge to the left and right
            nodes.back().inEdges.push_back(edge((i-1),i,conductance,(1.0f/p.R2)));
            nodes.back().inEdges.push_back(edge((i+1),i,conductance,(1.0f/p.R2)));    
        }
    }
}

/* Populate the specified node vector with an single-node RC vith values R1 and
 * C1. V1 is applied to N0 and allowed to decay. Other values in the params are 
 * not used.
 *
 * N.B. nodeCount is overwritten to be 1 for this generator
 *
 *        N0
 *    ┌────┴───┐ 
 *    │        │
 *  ┌─┴─┐      │
 *  │   │      │
 *  │   │R1   ═══ C1
 *  │   │      │
 *  └─┬─┘      │
 *    │        │
 *    └────┬───┘
 *        GND
 */
void decayGen(std::vector<node>& nodes, uint32_t& nodeCount, params p, float tStep)
{
    nodeCount = 1;
    
    // Device has one conductance and one reactive to ground.
    float Rgnd = (p.C1/tStep);
    float iZx = 1.0f/((1.0f/p.R1) + (p.C1/tStep));
    
    nodes.push_back(node(0,iZx,Rgnd,0));
    nodes[0].Vxt1 = p.V1;
}

inline uint32_t xy2i(uint32_t x, uint32_t y, uint32_t dimension)
{
    return x + (y * dimension);
}

/* Populate the specified node vector with a 2D grid of dimension <nodeCount>
 * This involves two resistor values (R1 and R2) and one capacitor value (C1). 
 * Initial voltages and other values in the params are not used.
 */
void gridGen(std::vector<node>& nodes, uint32_t& nodeCount,
                    params p, float tStep)
{
    for(uint32_t y = 0; y < nodeCount; y++)
    {
        for(uint32_t x = 0; x < nodeCount; x++)
        {
            if(x==0 && y==0)
            {   // bottom left corner, has a current source, R & C to Gnd, and R to N1.
                float iZx = 1.0f/((1.0f/p.R1) + 2*(1.0f/p.R2) + (p.C1/tStep));
                float Rgnd = (p.C1/tStep);
                
                nodes.push_back(node(0,iZx,Rgnd,1,twoS));
                
                // Add a "conductance" edge East: to node 0,1 = N1
                nodes.back().inEdges.push_back(edge(xy2i(1,0,nodeCount),0,conductance,(1.0f/p.R2)));
                
                // Add a "conductance" edge North
                nodes.back().inEdges.push_back(edge(xy2i(0,1,nodeCount),0,conductance,(1.0f/p.R2)));
            }
            
            else if(x==(nodeCount-1) && y==(nodeCount-1))
            {   // top right corner, has a current source, R & C to Gnd, and R.
                float iZx = 1.0f/((1.0f/p.R1) + 2*(1.0f/p.R2) + (p.C1/tStep));
                float Rgnd = (p.C1/tStep);
                
                nodes.push_back(node(xy2i(x,y,nodeCount),iZx,Rgnd,1,invertedtwoS));
                
                // Add a "conductance" edge West: to node 0,1 = N1
                nodes.back().inEdges.push_back(edge(xy2i((x-1),y,nodeCount),
                                                    xy2i(x,y,nodeCount),
                                                    conductance,(1.0f/p.R2)));
                
                // Add a "conductance" edge South
                nodes.back().inEdges.push_back(edge(xy2i(x,(y-1),nodeCount),
                                                    xy2i(x,y,nodeCount),
                                                    conductance,(1.0f/p.R2)));
            }
            
            else
            {   // "normal" nodes. R to NESW, C to GND
                float iZx = (p.C1/tStep);
                float Rgnd = (p.C1/tStep);
                
                node nd = node(xy2i(x,y,nodeCount),0,Rgnd,0);
                
                
                if(x<(nodeCount-1))
                {   // we have a connection to EAST
                    nd.inEdges.push_back(edge(xy2i((x+1),y,nodeCount),
                                                xy2i(x,y,nodeCount),
                                                conductance, (1.0f/p.R2)));
                    iZx += (1.0f/p.R2);
                }
                if(y<(nodeCount-1))
                {   // we have a connection to NORTH
                    nd.inEdges.push_back(edge(xy2i(x,(y+1),nodeCount),
                                                xy2i(x,y,nodeCount),
                                                conductance, (1.0f/p.R2)));
                    iZx += (1.0f/p.R2);
                }
                if(x>0)
                {   // we have a connection to WEST
                    nd.inEdges.push_back(edge(xy2i((x-1),y,nodeCount),
                                                xy2i(x,y,nodeCount),
                                                conductance, (1.0f/p.R2)));
                    iZx += (1.0f/p.R2);
                }
                if(y>0)
                {   // we have a connection to SOUTH
                    nd.inEdges.push_back(edge(xy2i(x,(y-1),nodeCount),
                                                xy2i(x,y,nodeCount),
                                                conductance, (1.0f/p.R2)));
                    iZx += (1.0f/p.R2);
                }
                
                // Do the divide!
                nd.iZx = 1/iZx;
                
                nodes.push_back(nd);
            }
        }
    }
}



void generate(uint32_t nodeCount, float tStep, float tMax, params iparams, 
                genType_t gType, std::string oFname, bool constrain,
                bool eightbox)
{
    uint32_t nStep =  static_cast<uint32_t>(tMax / tStep);
    if(fmod(tMax,tStep) != 0)
    {   // non-divisible step, extend the simulation by a timestep
        nStep++;
    }
    
    uint32_t deviceCount = nStep*nodeCount;
    
    std::vector<node> nodes;
    
    // Populate the nodes vector
    switch(gType)
    {
      case decay: 
                decayGen(nodes, nodeCount, iparams, tStep);
                break;
      
      case transmission:
                tlineGen(nodes, nodeCount, iparams, tStep);
                break;
      
      case grid:
                gridGen(nodes, nodeCount, iparams, tStep);
                deviceCount *= nodeCount;   // Tweak deviceCount for the grid
                nodeCount *= nodeCount;     // Tweak nodeCount for the grid
                break;
      
      default:
                return;
    }
    
    unsigned idleIdxScale = 1000;
    //if(deviceCount<75000) {
    //    idleIdxScale = 10000;
    //} else {
    //    idleIdxScale = 30000;
    //} 
    
    std::cout << "Generating Reactive XML:" << std::endl;
    std::cout << "\t node count=\t\t" << nodeCount << std::endl;
    std::cout << "\t simulated time=\t" << tMax << "s" << std::endl;
    std::cout << "\t timestep=\t\t" << tStep << "s" << std::endl;
    std::cout << "\t number of steps=\t" << nStep << std::endl;
    std::cout << "\t number of devices=\t" << deviceCount << std::endl;
    std::cout << "\t idleMax =\t\t" << idleIdxScale << std::endl;
    
    
    
    std::ofstream oFile, cFile;
    oFile.open("./"+oFname+".xml");
    
    // Write the preamble
    oFile << "<?xml version=\"1.0\"?>\n";
    oFile << "<!-- Generated using the POETS Orchestrator Reactive proof of ";
    oFile << "concept.\n-->\n";
    oFile << "<Graphs xmlns=\"\" appname=\"Reactive6\">\n";
    
    
    
    
    // Copy the type.
    //TODO:
    std::ifstream tFile;
    tFile.open("./reactive_type.xml");    //Type file
    std::string line;
    while(std::getline(tFile, line))
    {
        std::size_t NODE_pos, STEP_pos, DEV_pos, TSTEP_pos, IDMAX_pos;
        
        std::string NODE_str ("NODE_DEF");
        std::string STEP_str ("STEP_DEF");
        std::string DEV_str ("DEV_DEF");
        std::string TSTEP_str ("TS_DEF");
        std::string IDMAX_str ("IDMAX_DEF");
        
        NODE_pos = line.find(NODE_str);
        if (NODE_pos != std::string::npos){
            // Line contains NODE_DEF - replace with nodeCount
            line.replace(NODE_pos, NODE_str.length(), std::to_string(nodeCount));
        }
        
        STEP_pos = line.find(STEP_str);
        if (STEP_pos != std::string::npos){
            // Line contains STEP_DEF - replace with nStep
            line.replace(STEP_pos, STEP_str.length(), std::to_string(nStep));
        }
        
        DEV_pos = line.find(DEV_str);
        if (DEV_pos != std::string::npos){
            // Line contains DEV_DEF - replace with nStep*nodeCount
            line.replace(DEV_pos, DEV_str.length(), std::to_string(nStep*nodeCount));
        }
        
        TSTEP_pos = line.find(TSTEP_str);
        if (TSTEP_pos != std::string::npos){
            // Line contains TS_DEF - replace with tStep
            line.replace(TSTEP_pos, TSTEP_str.length(), std::to_string(tStep));
        }

        IDMAX_pos = line.find(IDMAX_str);
        if (IDMAX_pos != std::string::npos){
            // Line contains IDMAX_DEF - replace with idleIdxScale
            line.replace(IDMAX_pos, IDMAX_str.length(), std::to_string(idleIdxScale));
        }
        
        oFile << line << std::endl;
    }
    tFile.close();
    
    
    /* Now to write out the actual graph instance. This needs to iterate over
     * timesteps twice: once for the device instances and once for the edge
     * instances.
     */
    oFile << "  <GraphInstance id=\"reactive6\" graphTypeId=\"reactive\" ";
    oFile << "P=\"{" << tStep << "}\">\n";
    
    oFile << "\t<DeviceInstances>\n";   
    // For each timestep, write out the device instances.
    for(uint32_t i = 0; i < nStep; i++)
    {
        // Iterate over the nodes and write them out
        for(std::vector<node>::iterator nodeIt = nodes.begin();
            nodeIt != nodes.end(); nodeIt++)
        {
            (*nodeIt).writeNode(i,(i*tStep), oFile);
        }
    }
    oFile << "\t</DeviceInstances>\n";
    
    oFile << "\t<EdgeInstances>\n";
    // For each timestep, write out the edge instances.
    for(uint32_t i = 0; i < nStep; i++)
    {
        // Iterate over the nodes
        for(std::vector<node>::iterator nodeIt = nodes.begin();
            nodeIt != nodes.end(); nodeIt++)
        {
            // Iterate over the node's edges and write them out
            for(std::vector<edge>::iterator edgeIt = (*nodeIt).inEdges.begin();
                edgeIt != (*nodeIt).inEdges.end(); edgeIt++)
            {
                (*edgeIt).writeEdge(i, oFile);
            }
            
            if(i>0)
            {   // The device has a connection to the past, write it
                edge old = edge((*nodeIt).id,(*nodeIt).id,past,0.0f);
                old.writeEdge(i, oFile);
            }
        }
        
        oFile << "\n";      // Add a little bit of space.
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
    if(constrain)
    {
        cFile << "place /constraint = \"MaxDevicesPerThread\", ";
        uint32_t tCount = 6144;
        if(eightbox) tCount = 49152;
        cFile << (((deviceCount%tCount)>0)?((deviceCount/tCount)+1):(deviceCount/tCount));
        cFile << "\n";
    }
    cFile << "place /bucket = *\n";
    cFile << "compose /nore = *\n";
    cFile << "compose /app = *\n";
    cFile << "deploy /app = *\n";
    cFile << "initialise /app = *\n";
    cFile << "run /app = *\n";
    std::cout << "Call file" << std::endl;
    cFile.close();
}


int main(int argc, const char * argv[])
{   
    float tMax = 6.0f;              // 6 seconds run time
    float tStep = 0.01;
    uint32_t nodeCount = 6;     // Transmission chain of 6 nodes
    genType_t gType = transmission;
    params gParams = {100.0f, 1000.0f, 0.0001f, 0.0f, 0.0f, 0.0f};
    bool constrain = false;
    bool eightbox = false;
    
    std::string oFname;
    //params dparams = {1000.0f, 0.0f, 0.0001f, 0.0f, 100,0f, 0.0f};    // Decay default
    //params tparams = {100.0f, 1000.0f, 0.0001f, 0.0f, 0,0f, 0.0f};    // TL default
    
    //Handle arguments for generation
    args::ArgumentParser parser("Application: plate_heat generator, POETS Project\n");
    parser.LongSeparator("=");
    args::HelpFlag help(parser, "help", "Display this help menu", {'h', "help"});
    args::Group optional(parser, "All of these arguments are optional:", args::Group::Validators::DontCare);
    args::Group oneof(parser, "At most one of these options:", args::Group::Validators::AtMostOne);
    
    
    args::ValueFlag<int> ncIn(optional, "int", "Set node count default=6", {'n', "nodeCount"});
    args::ValueFlag<float> stepIn(optional, "float", "Set the simulation timestep, default 0.01s", {'t'});
    args::ValueFlag<float> tmaxIn(optional, "float", "Set the simulation duration, default 6s", {'d'});
    
    args::Flag tlIn(oneof, "Transmission Line", "Generate a transmission line", {"transmission"});
    args::Flag dcIn(oneof, "RC Decay", "Generate an RC decay", {"decay"});
    args::Flag gdIn(oneof, "RC Grid", "Generate a grid of resistors and capacitors", {"grid"});
    
    args::ValueFlag<float> r1In(optional, "float", "Set the R1 parameter for the simulation. Default = 100 Ohms", {"R1"});
    args::ValueFlag<float> r2In(optional, "float", "Set the R2 parameter for the simulation. Default = 1000 Ohms", {"R2"});
    args::ValueFlag<float> c1In(optional, "float", "Set the C1 parameter for the simulation. Default = 0.0001 Farads", {"C1"});
    args::ValueFlag<float> c2In(optional, "float", "Set the C2 parameter for the simulation. Default = 0 Farads", {"C2"});
    args::ValueFlag<float> v1In(optional, "float", "Set the V1 parameter for the simulation. Default = 0 V", {"V1"});
    args::ValueFlag<float> v2In(optional, "float", "Set the V2 parameter for the simulation. Default = 0 V", {"V2"});
    
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
    
    // Set the nodeCount
    if(ncIn) 
    {
        nodeCount = args::get(ncIn);
    }
    
    // Set the simulation timestep
    if(stepIn) 
    {
        tStep = args::get(stepIn);
    }
    
    // Set the simulation duration
    if(tmaxIn) 
    {
        tMax = args::get(tmaxIn);
    }
    
    // Set the underlying generator.
    if(tlIn)            gType = transmission;
    else if(dcIn)       gType = decay;
    else if(gdIn)       gType = grid;
    
    if(cnIn)            constrain = true;
    if(ebIn)            eightbox = true;
    
    // Sort out the output filename
    oFname = "reactive";
    if(oReplace)
    {
        oFname = args::get(oReplace);
    }
    else if(oAppend)
    {
        oFname += "_";
        oFname += args::get(oAppend);
    }
    
    
    // Get the params
    if(r1In)        gParams.R1 = args::get(r1In);
    if(r2In)        gParams.R2 = args::get(r2In);
    if(c1In)        gParams.V1 = args::get(c1In);
    if(c2In)        gParams.C2 = args::get(c2In);
    if(v1In)        gParams.V1 = args::get(v1In);
    if(v2In)        gParams.V2 = args::get(v2In);
    
    generate(nodeCount,tStep,tMax,gParams,gType,oFname,constrain,eightbox);
    return 0;
}
