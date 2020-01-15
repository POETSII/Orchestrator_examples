//------------------------------------------------------------------------------

#include <stdio.h>
#include "pdigraph.hpp"
#include "macros.h"
#include "flat.h"
#include "filename.h"
#include <math.h>
#include <string>
#include <vector>
#include <map>  // needed to replace supervisor properties. Could do without, but why?
#include <set>  // bodgey solution to get monitored neurons for POETS. See below.
using namespace std;

const unsigned delta_max = 15;
const unsigned delta_min = 1;
const unsigned omega_min = 1;
const unsigned omega_max = 2048;
const int IR_max = 1073741824;
const int V_max = 262144;
const int V_min = -262144;

// Real programmers can write FORTRAN in any language.....

//==============================================================================
// Global data declares
// "Template argument cannot have static or local linkage" - pathetic

struct N_t {                           // Neuron structure
  N_t(unsigned _r,unsigned _x,unsigned _y):
    r(_r),x(_x),y(_y){
      char buf[256];
      sprintf(buf,"R%02uX%03uY%03u",r,x,y);
      name = string(buf);
    }
  N_t(string n):r(0),x(0),y(0) { name = n; }
  unsigned r,x,y,p;
  string   name;
  string   param;
  void     Dump() { printf("[%s:%s]\n",name.c_str(),param.c_str()); }
  string   Name() { return name; }
};

struct Ne_pt {                         // Body neuron parameters
  Ne_pt():name("p0"),h(2.0),th(1.9),rf(0.001),dc(20),fd(0.002){}
  string   name;                       // Parameter set name
  unsigned Mt;                         // Internal (ANSWER) model type
  double   h;                          // Neuron output spike size
  double   th;                         // Neuron firing threshold
  double   rf;                         // Neuron refractory period
  double   dc;                         // Leak rate
  double   fd;                         // Fire delay
  void APrnt(FILE * fp) {              // For ANSWER
    fprintf(fp,"!%s %u %e %e %e %e %e\n",name.c_str(),Mt,h,th,rf,dc,fd);
  }
  void UPrnt(FILE * fp) {              // For Loader103
    // fprintf(fp,"*neuron : %s(\"\",\"%32b%32b%32b%32b%32b%32b\")\n",name.c_str());
    // above syntax looks wrong - guessing below
    fprintf(fp,"*neuron : %s(\"\",\"%%32b%%32b%%32b%%32b%%32b%%32b\")\n",name.c_str());
    fprintf(fp,"#neuron : %s(%u,%e,%e,%e,%e,%e)\n",name.c_str(),Mt,h,th,rf,dc,fd);
  }
  void PPrnt(FILE * fp, unsigned addr, double tsf, unsigned rec) {  // For POETS
    int Vth_scaled = static_cast<int>(10240*th-66560);
    unsigned omega_scaled = static_cast<unsigned>(1024*(100/(exp(1)*dc))/tsf);
    omega_scaled = omega_scaled > omega_max ? omega_max : (omega_scaled < omega_min ? omega_min : omega_scaled);
    // unsigned omega_scaled = static_cast<unsigned>(1000*(log(100)-log(100-dc))/tsf);
    fprintf(fp,"<P>\"addr\": %u, \"theta\": %d, \"tauRefrac\": %u, \"omega\": %u, \"record\": %u</P>",
	    addr, Vth_scaled,
	    ~(0xFFFFFFFF >> static_cast<unsigned>(rf*tsf)),
	    omega_scaled, rec);
  }
  void PSPrnt(FILE *fp, double tsf, unsigned omega_post) { // For POETS (synaptic data)
    // printf("Calculating delta : fire delay = %f, timescale factor = %f\n", fd, tsf);
    unsigned delta_scaled = static_cast<unsigned>(fd*tsf);
    // printf("Computed delta %u, float delta %f\n", delta_scaled, fd*tsf);
    int IR_scaled = static_cast<int>(10485760*h/omega_post);
    IR_scaled = IR_scaled > IR_max ? IR_max : IR_scaled;
    delta_scaled = delta_scaled > delta_max ? delta_max : (delta_scaled < delta_min ? delta_min : delta_scaled);
    fprintf(fp,"        <P>\"delta\": %u</P><S>\"IR_syn\": %d</S>\n",
	    delta_scaled, IR_scaled);
  }
} Ne_p;


/* awkward - need to set up a new parameter set container for POETS
   because the ANSWER format expects parameters at the end of the
   output file, labelled by a tag value, whereas POETS expects
   parameters at the DeviceInstance instantiation. But meanwhile,
   some of the parameters for a *source* neuron in ANSWER actually
   correspond to synapse parameters in POETS, which relate to the
   *target* edge. And EdgeInstances happen after DeviceInstances in
   the file, so modifying a single parameter value there would affect
   all the edges being instantiated. The output generators print
   directly to a file, which could be very big, so we don't want 
   to keep large structures in memory or create multiple files and
   then mash them together, so this seems to be the most expedient
   way out. 
*/ 
struct NeO_pt {                        // Output neuron parameters
  NeO_pt():name("px"),h(2.0),th(1.9),rf(0.001),dc(1000.0),fd(0.001){}
  string   name;                       // Parameter set name
  unsigned Mt;                         // Internal (ANSWER) model type
  double   h;                          // Neuron output spike size
  double   th;                         // Neuron firing threshold
  double   rf;                         // Neuron refractory period
  double   dc;                         // Leak rate
  double   fd;                         // Fire delay
  void APrnt(FILE * fp) {              // For ANSWER
    fprintf(fp,"!%s %u %e %e %e %e %e\n",name.c_str(),Mt,h,th,rf,dc,fd);
  }
  void UPrnt(FILE * fp) {              // For Loader103
    fprintf(fp,"*neuron : %s(\"\",\"%%32b%%32b%%32b%%32b%%32b%%32b\")\n",name.c_str());
    fprintf(fp,"#neuron : %s(%u,%e,%e,%e,%e,%e)\n",name.c_str(),Mt,h,th,rf,dc,fd);
  }
  void PPrnt(FILE * fp, unsigned addr, double tsf) {  // For POETS
    int Vth_scaled = static_cast<int>(10240*th-66560);
    unsigned omega_scaled = static_cast<unsigned>(1024*(100/(exp(1)*dc))/tsf);
    omega_scaled = omega_scaled > omega_max ? omega_max : (omega_scaled < omega_min ? omega_min : omega_scaled);
    //unsigned omega_scaled = static_cast<unsigned>(1000*(log(100)-log(100-dc))/tsf);
    fprintf(fp,"<P>\"addr\": %u, \"theta\": %d, \"tauRefrac\": %u, \"omega\": %u, \"record\": 1</P>",
	    addr, Vth_scaled,
	    ~(0xFFFFFFFF >> static_cast<unsigned>(rf*tsf)),
	    omega_scaled);
  }
  void PSPrnt(FILE *fp, double tsf, unsigned omega_post) { // For POETS (synaptic data)
    unsigned delta_scaled = static_cast<unsigned>(fd*tsf);
    int IR_scaled = static_cast<int>(10485760*h/omega_post);
    IR_scaled = IR_scaled > IR_max ? IR_max : IR_scaled;
    delta_scaled = delta_scaled > delta_max ? delta_max : (delta_scaled < delta_min ? delta_min : delta_scaled);
    fprintf(fp,"        <P>\"delta\": %u</P><S>\"IR_syn\": %d</S>\n",
	    delta_scaled, IR_scaled);
  }
} NeO_p;

struct So_pt {                         // Source parameters
  So_pt():name("pSRC"),h(2.0),ts(0.002),Tmin(1.0),Nmin(1),Tmaj(100000.0),Nmaj(1){}
  string   name;                       // Parameter set name
  double   h;                          // Height of output pulse
  double   ts;                         // Start delay
  double   Tmin;                       // T minor - spike spacing within bursts
  unsigned Nmin;                       // N minor - number of spikes in a burst
  double   Tmaj;                       // T major - spacing between the beginnings of bursts
  unsigned Nmaj;                       // N major - number of bursts
  void APrnt(FILE * fp) {              // For ANSWER
    fprintf(fp,"!%s %e %e %e %u %e %u\n",name.c_str(),h,ts,Tmin,Nmin,Tmaj,Nmaj);
  }
  string sPrnt() {                     // For Loader103
    char buf[256];
    sprintf(buf,"%s(%e,%e,%e,%u,%e,%u)",name.c_str(),h,ts,Tmin,Nmin,Tmaj,Nmaj);
    return string(buf);
  }
  void UPrnt(FILE * fp) {              // For Loader103
    fprintf(fp,"*device : %s (\"\",\"%%32b%%32b%%32b%%32b%%32b%%32b\")\n",name.c_str());
  }
  void PPrnt(FILE * fp, unsigned addr) { // for POETS. This only generates a single spike for the moment
    fprintf(fp,"<P>\"addr\": %u, \"record\": 1</P>",addr);
    //fprintf(fp,"<P>\"addr\": %u, \"record\": 1</P><S>\"V\": -40960</S>",addr);
  }
  void PSPrnt(FILE * fp, double tsf, unsigned omega_post) { // for POETS (synaptic data)
    unsigned delta_scaled = static_cast<unsigned>(ts*tsf);
    int IR_scaled = static_cast<int>(10485760*h/omega_post);
    IR_scaled = IR_scaled > IR_max ? IR_max : IR_scaled;
    delta_scaled = delta_scaled > delta_max ? delta_max : (delta_scaled < delta_min ? delta_min : delta_scaled);
    fprintf(fp,"        <P>\"delta\": %u</P><S>\"IR_syn\": %d</S>\n",
	    delta_scaled, IR_scaled);    
  }
} So_p;

struct Si_pt {                         // Sink parameters
  Si_pt():name("pSINK"),th(1.0),dc(1e-5){}
  string   name;                       // Parameter set name
  double   th;                         // Neuron firing (ie. killing) threshold
  double   dc;                         // Leak rate
  void APrnt(FILE * fp) {              // For ANSWER
    fprintf(fp,"!%s %e %e\n",name.c_str(),th,dc);
  }
  string sPrnt() {                     // For Loader103
    char buf[256];
    sprintf(buf,"%s(%e,%e)",name.c_str(),th,dc);
    return string(buf);
  }
  void UPrnt(FILE * fp) {              // For Loader103
    fprintf(fp,"*device : %s (\"\",\"%%32b%%32b%%32b%%32b%%32b%%32b\")\n",name.c_str());
  }
  void PPrnt(FILE * fp, unsigned addr, double tsf) { // for POETS.
    int Vth_scaled = static_cast<int>(10240*th); // -66560); kill off spikes with a very high threshold
    unsigned omega_scaled = static_cast<unsigned>(1024*(100/(exp(1)*dc))/tsf);
    Vth_scaled = Vth_scaled > V_max ? V_max : (Vth_scaled < V_min ? V_min : Vth_scaled);
    omega_scaled = omega_scaled > omega_max ? omega_max : (omega_scaled < omega_min ? omega_min : omega_scaled);
    fprintf(fp,"<P>\"addr\": %u, \"theta\": %d, \"omega\": %u</P>",
	    addr, Vth_scaled,
	    omega_scaled);
  }
} Si_p;

struct Cl_pt {                         // Clock parameters
  Cl_pt():name("pCLK"),ts(0.001),tick(0.0005){}
  string   name;                       // Parameter set name
  double   ts;                         // Start delay
  double   tick;                       // Tick interval
  void APrnt(FILE * fp) {              // For ANSWER
    fprintf(fp,"!%s %e %e\n",name.c_str(),ts,tick);
  }
} Cl_p;
                                       // Circuit graph
pdigraph<string,N_t *,int,int,int,int> G;
unsigned Rsize[] = {3,5,7,11,13,17,19};// Synfire ring sizes
vector<string> vMon;                   // List of nodes to be monitored
string sInput("Input");
string sOutput("Output");
string sSource("Source");
string sSink("Sink");
unsigned Mt;                           // Ring body neuron model type

//==============================================================================
// Forward routine declares

void AD_cb(int const &);
void AK_cb(int const &);
void BuildG(unsigned,unsigned,double,unsigned);
void ND_cb(N_t * const &);
void NK_cb(string const &);
unsigned TotalDelay(unsigned);
void WriteANSWER(string,string,unsigned);
void WritePOETS(FileName*,unsigned);
void WriteUIF(string,unsigned);

//==============================================================================

int main(int argc, char* argv[])
{
printf("Hello, %lu-bit world\n",sizeof(unsigned)*8);
if (argc<6) {
  printf("Requires: "
         "root_filename ring_count ring_width pool_connect_prob neuron_type\n"
         "Hit any key to boogey\n");
  getchar();
  return 1;
}
long t0 = mTimer();                    // Start wallclcok
printf("[Synfire ring builder 1 launched as %s\n",argv[0]);
FileName Fn(argv[1]);
Fn.FNExtn("cct");                      // Force the circuit extension
string cname = Fn.FNComplete();        // File for ANSWER circuit
Fn.FNExtn("mon");                      // Force the monitor list extension
string mname = Fn.FNComplete();        // File for ANSWER monitor list
Fn.FNExtn("ecct");                     // Force UIF circuit file extension
string ename = Fn.FNComplete();
Fn.FNExtn("xml");                      // Force POETS file extension

unsigned R   = str2uint(argv[2]);      // Ring count
unsigned W   = str2uint(argv[3]);      // Ring width
double   Pr  = str2dble(argv[4]);      // Pool-pool connection probability
         Mt  = str2uint(argv[5]);      // Neuron model type

if (R*sizeof(unsigned)>sizeof(Rsize)) {// Save the user from themself
  R = sizeof(Rsize)/sizeof(unsigned);
  printf("R truncated to %u\n",R);
}
if ((Mt==0)||(Mt>2)) {
  Mt = 1;
  printf("Mt overriden to type 1\n");
}

printf("\nSynfire ring build parameters:\n");
printf("Pool width %u\n",W);
printf("Pool-pool interconnect probability %5.2f\n",Pr);
printf("\n%u rings:\n",R);
for(unsigned i=0;i<R;i++) printf("Ring %u size %u\n",i,Rsize[i]);
printf("Neuron model type %u\n",Mt);

G.SetND_CB(ND_cb);                     // Digraph dump callbacks
G.SetNK_CB(NK_cb);
G.SetAD_CB(AD_cb);
G.SetAK_CB(AK_cb);

BuildG(R,W,Pr,Mt);                     // Populate the graph
t0 = mTimer(t0);                       // End of time
printf("... synfire ring built in %ld wall ms]\n\n",t0);
// have to generate POETS file first because the ANSWER generator
// fiddles with the Ne_p parameter values.
WritePOETS(&Fn,R);                     // Generate the POETS file 
WriteANSWER(cname,mname,R);            // Dump out in ANSWER format
WriteUIF(ename,R);                     // Guess
                                       // Tidy up
WALKPDIGRAPHNODES(string,N_t *,int,int,int,int,G,i) delete G.NodeData(i);

printf("\nSmite any key to decamp\n");
getchar();
return 0;
}

//------------------------------------------------------------------------------

void AD_cb(int const & i)
{
printf("[AD:%d]",i);
}

//------------------------------------------------------------------------------

void AK_cb(int const & i)
{
printf("[AK:%d]",i);
}

//------------------------------------------------------------------------------

void BuildG(unsigned R,unsigned W,double Pr,unsigned Mt)
// Populate the circuit graph AND build the monitor list for the ANSWER simulator
{
unsigned index=0;                      // ID for the arcs
vector<vector<N_t *> > vvN;            // Rectangular matrix of neurons
vector<N_t *> vN;                      // Column vector
vector<pair<N_t *,N_t *> > p2N;        // Somewhere to store the I/O
string sp0("p0");                      // Bulk parameter set
string spx("px");                      // Collector parameter set at the end

N_t * pN;                              // Neuron pointer
for (unsigned r=0;r<R;r++) {           // One ring at a time
  vvN.clear();                         // Clear the ring matrix
  printf("Ring %u, size %u\n",r,Rsize[r]);
  for (unsigned x=0;x<Rsize[r];x++) {  // One pool (column) at a time
    vN.clear();                        // Clear the pool vector
    printf("Column %u\n",x);
    for (unsigned w=0;w<W;w++) {
      pN = new N_t(r,x,w);
      pN->param = sp0;
//      pN->Dump();
      vN.push_back(pN);                 // Store the new neuron in the column
    }
    vMon.push_back(pN->Name());         // Store the lst one to be monitored
    vvN.push_back(vN);                  // Store the completed column
  }
  for(vector<vector<N_t *> >::iterator i=vvN.begin();i!=vvN.end();i++) {
    WALKVECTOR(N_t *,(*i),j) {
//      printf("Inserting %s\n",(*j)->Name().c_str());
      G.InsertNode((*j)->Name(),*j);    // Shove into main graph
    }
  }
                                        // Save I/O neurons
  p2N.push_back(pair<N_t *,N_t *>(vvN[0][0],vvN[Rsize[r]-1][0]));

  for(unsigned x=0;x<Rsize[r]-1;x++) {  // Cross link columns in the matrix
    WALKVECTOR(N_t *,vvN[x],j) {
      WALKVECTOR(N_t *,vvN[x+1],k) {
        if (!P(Pr)) continue;
//        printf("Linking %s to %s\n",(*j)->Name().c_str(),(*k)->Name().c_str());
        G.InsertArc(index++,(*j)->Name(),(*k)->Name());
      }
    }
  }
  WALKVECTOR(N_t *,vvN[0],j) {          // Close the loop in the matrix
    WALKVECTOR(N_t *,vvN[Rsize[r]-1],k) {
      if (!P(Pr)) continue;
//      printf("...and linking %s to %s\n",
 //            (*k)->Name().c_str(),(*j)->Name().c_str());
      G.InsertArc(index++,(*k)->Name(),(*j)->Name());
    }
  }
}
// So now we have a set of disjoint rings; join them up:
printf("...join the rings\n");
                                        // Connect up all the I/O
pN = new N_t(sInput);                   // Input node
pN->param = sp0;                        // Parameter set
G.InsertNode(sInput,pN);                // Insert into graph
vMon.push_back(sInput);                 // Insert into monitor list
pN = new N_t(sOutput);                  // Output
pN->param = spx;                        // Parameters are different
G.InsertNode(sOutput,pN);               // Insert into graph
vMon.push_back(sOutput);                // Insert into monitor list

for(vector<pair<N_t *,N_t *> >::iterator i=p2N.begin();i!=p2N.end();i++) {
  printf("....and linking %s to %s\n",sInput.c_str(),(*i).first->Name().c_str());
  G.InsertArc(index++,sInput,(*i).first->Name());
  printf("....and linking %s to %s\n",(*i).second->Name().c_str(),sOutput.c_str());
  G.InsertArc(index++,(*i).second->Name(),sOutput);
}

//G.Dump();

printf("\nSynfire ring set complete\n");
printf("\nTotal nodes %u\nTotal arcs %u\n",G.SizeNodes(),G.SizeArcs());
}

//------------------------------------------------------------------------------

void ND_cb(N_t * const & pD)
{
printf("[NK:%s]",pD->Name().c_str());
}

//------------------------------------------------------------------------------

void NK_cb(string const & u)
{
printf("[NK:%s]",u.c_str());
}

//------------------------------------------------------------------------------

unsigned TotalDelay(unsigned R)
{
if (R > 7) {
   printf("Total number of rings requested %d exceeds current limit of 7\n",R);
return 0;
}
unsigned delay = 1;
for (unsigned d=0; d<R; d++) delay *= Rsize[d];
return delay;
}

//------------------------------------------------------------------------------

void WriteANSWER(string cname,string mname,unsigned R)
{
long t0 = mTimer();
printf("\n[Writing ANSWER circuit description files...\n");

string clk  = "clk";
//string srce = "srce";
//string sink = "sink";

FILE * fp = fopen(cname.c_str(),"w");
if (fp == NULL)
{
   printf("Couldn't open output file %s: ANSWER file not generated\n", cname.c_str());
   return;
}
                                       // Devices
WALKPDIGRAPHNODES(string,N_t *,int,int,int,int,G,i) {
  N_t * N = G.NodeData(i);             // Walk the devices
  fprintf(fp,"%s %s ",N->Name().c_str(),N->param.c_str());  // Driving node....
  vector<string> vNk;
  G.DrivenNodes(N->Name(),vNk);        // Extract driven nodes
  if (N->Name()==sOutput) fprintf(fp,"%s ",sSink.c_str());
  WALKVECTOR(string,vNk,j) fprintf(fp,"%s ",(*j).c_str());
  fprintf(fp,"\n");
}
                                       // Parameter sets
Ne_p.Mt = Mt;                          // Neuron model type
Ne_p.APrnt(fp);                        // Print parameter set
                                       // For the output neuron:
Ne_p.name = "px";                      // Modify name
Ne_p.th = 0.95 * Ne_p.h * R;           // Modify threshold
Ne_p.dc = 1000.0;                      // Modify leak rate
Ne_p.fd = 0.000002;                    // Modify fire delay
Ne_p.APrnt(fp);                        // And print it
if (Mt==1) {                           // Clock needed?
  fprintf(fp,"*%s %s\n",clk.c_str(),Cl_p.name.c_str());// Clock device
  Cl_p.APrnt(fp);                      // Clock parameters
}
                                       // Connect source device
fprintf(fp,">%s %s %s\n",sSource.c_str(),So_p.name.c_str(),sInput.c_str());
So_p.APrnt(fp);                        // Source parameters
fprintf(fp,"<%s %s\n",sSink.c_str(),Si_p.name.c_str());// Connect sink device
Si_p.APrnt(fp);                        // Sink parameters

unsigned D=1;                          // Calculate ample simulation time
for(unsigned i=0;i<R;i++) D*=Rsize[i];
double stop = So_p.ts + (double(D) + 2.0 * Ne_p.fd) * 1.05;
fprintf(fp,"== %e\n",stop);            // Simulation stop time
fclose(fp);                            // End of circuit file
printf("Monitor list (%s) contains %lu devices\n",mname.c_str(),vMon.size());
                                       // Now write the monitor list
vMon.push_back(clk);
vMon.push_back(sSource);
vMon.push_back(sSink);
fp = fopen(mname.c_str(),"w");
WALKVECTOR(string,vMon,i) fprintf(fp,"%s\n",(*i).c_str());
fclose(fp);
t0 = mTimer(t0);
printf("...ANSWER circuit description files built in %ld wall ms]\n\n",t0);
}

//------------------------------------------------------------------------------

void WritePOETS(FileName* pname, unsigned R)
{
long t0 = mTimer();
printf("\n[Writing POETS synfire generator description file...\n");

string clk  = "clk";
//string srce = "srce";
//string sink = "sink";

// total number of neurons: size of the device graph plus source and sink.
unsigned num_neurons = G.SizeNodes() + 2;
// compute the time the simulation will need to run for - the product of the ring lengths and
// the delays
unsigned sim_time = TotalDelay(R); // this is the total ring length
double time_scale = 1/Cl_p.tick;   // adjust for clock period
sim_time += 1;                     // one more neuron for the input           
sim_time *= (time_scale*Ne_p.fd);  // scale the aggregate delay
sim_time += (time_scale*NeO_p.fd); // add output neuron with different delay
sim_time += (time_scale*So_p.ts);  // add some for the source
sim_time += 1000;                  // fudge factor - add some time

/* Bodge here - we want to set up neurons for recording (monitoring) when we
   create the DeviceInstance, but the monitor list is, for the moment, a vector
   of strings to the monitored device's name. The DeviceInstance is being
   created by walking through the graph device by device, so we have to compare
   the current device against the monitor list. But the original list being
   a vector, that would require a linear seach (bad!) so we transform it into
   a set by the neurons's node pointer (for slightly faster lookup than by
   searching on a string). Still not the most efficient, the best way to do
   this is to create a flag in the N_t type, but we are trying to avoid significant
   modifications to the rest of the cpp, we want to contain the changes to this
   one function as much as possible. 
*/
printf("Monitor list contains %lu devices\n",vMon.size()); // Build the monitor list
set<N_t*> monitored_neurons;                                       
WALKVECTOR(string,vMon,i)
{
   N_t** mn = G.FindNode(*i);
   if (mn != 0) monitored_neurons.insert(*mn);
} 
// needed because FileName::FNPath asks for a reference to a string vector
// (for reasons probably lost to the mists of history). We won't use it, anyway.
FileName tname = *pname;
tname.FNBase("SpiNN_lif_cur_eul_type");
// vector<string> discardPath;
FILE * fp = fopen(pname->FNComplete().c_str(),"w");
FILE * tp = fopen(tname.FNComplete().c_str(),"r");
if (fp == NULL)
{
   printf("Couldn't open output file %s: POETS file not generated\n", pname->FNComplete().c_str());
   return;
}
if (tp == NULL)
{
  printf("Application template %s not found: POETS file could not be generated\n", tname.FNComplete().c_str());
   return;
}
char xml_line_buf[512];            // buffer for reading in the GraphType template
char sup_var_buf[12];              // and for holding variable arguments
map<string, unsigned> sup_vars;    // search list for supervisor variables to replace 
sup_vars["SIMULATION_TIME"] = sim_time;
sup_vars["POPULATION_SIZE"] = monitored_neurons.size()+1; // 1 extra neuron for the source
sup_vars["POPULATION_MAX_IDX"] = num_neurons-1;
sup_vars["POPULATION_MIN_IDX"] = 0;
fputs("<?xml version=\"1.0\"?>\n", fp);
fputs("<Graphs xmlns=\"https://poets-project.org/schemas/virtual-graph-schema-v2\">\n", fp);
while (!feof(tp))
{
      if (fgets(xml_line_buf,512,tp) == NULL) break;
      string xml_line(xml_line_buf); // suck in the lines of the GraphType template.
      size_t var_pos;
      map<string, unsigned>::iterator v = sup_vars.begin();
      while (v != sup_vars.end())
      {
	 if ((var_pos = xml_line.find(v->first)) != string::npos) // look for the supervisor variables
         {
	    sprintf(sup_var_buf,"%u",v->second);
	    xml_line.replace(var_pos,v->first.length(),sup_var_buf); // replace with values if found
	    break;
         }
	 v++;
      }
      fputs(xml_line.c_str(), fp); // and dump the line with any replacements to the POETS xml file.
}
fclose(tp);
fputs("\n\n",fp);
fprintf(fp,"<GraphInstance id=\"%s\" graphTypeId=\"SpiNN_lif_cur_eul\">\n",pname->FNBase().c_str());
fprintf(fp,"    <Properties>\"endTime\": %u</Properties>\n",sim_time); // write the simulation time
fprintf(fp,"    <DeviceInstances>\n");   // Start the device instances section
Ne_p.Mt = Mt;                            // Neuron model type
NeO_p.Mt = Mt;                           // Same for output neuron
string nu_type = "LIFNeuron";            // preload the string; easier than testing
if (Mt == 2) nu_type = "LIFNeuronEvent";
string src_type = "SpikeSource";
NeO_p.th = 0.95 * Ne_p.h * R;          // Modify threshold for the output neuron
unsigned dev_idx = 0;                  // Application neuron index
                                       // Devices
WALKPDIGRAPHNODES(string,N_t *,int,int,int,int,G,i) {
  N_t * N = G.NodeData(i);             // Walk the devices
  unsigned record = 0;
  if (monitored_neurons.find(N) != monitored_neurons.end()) record = 1;         // are we recording? (inefficient)
  fprintf(fp,"      <DevI id=\"%s\" type=\"%s\">",N->Name().c_str(),nu_type.c_str()); // new DeviceInstance
  if (N->param == "px") NeO_p.PPrnt(fp,dev_idx++,time_scale);                   // Output device parameters
  else Ne_p.PPrnt(fp,dev_idx++,time_scale,record);                              // all other devices
  fprintf(fp,"</DevI>\n");                                                      // end DeviceInstance
}
 
fprintf(fp,"      <DevI id=\"%s\" type=\"%s\">",sSource.c_str(),src_type.c_str()); // Source DeviceInstance
So_p.PPrnt(fp,dev_idx++);                                                         // Source device parameters
fprintf(fp,"</DevI>\n");                                                          // end DeviceInstance
fprintf(fp,"      <DevI id=\"%s\" type=\"%s\">",sSink.c_str(),nu_type.c_str());   // Sink DeviceInstance
Si_p.PPrnt(fp,dev_idx++,time_scale);                                              // Source device parameters
fprintf(fp,"</DevI>\n");                                                          // end DeviceInstance
fprintf(fp,"    </DeviceInstances>\n");                                           // end DeviceInstances section

fprintf(fp,"    <EdgeInstances>\n");                                              // start the edge instances section

WALKPDIGRAPHNODES(string,N_t *,int,int,int,int,G,i) {
  N_t * N = G.NodeData(i);                                                        // walk devices for edges
  vector<string> vNk;
  G.DrivenNodes(N->Name(),vNk);        // Extract driven nodes
  if (N->Name()==sOutput)
  {
     fprintf(fp,"      <EdgeI path=\":SpikeReceiver-%s:Axon_Sup\"/>\n",sOutput.c_str());         // Connect output to supervisor
     fprintf(fp,"      <EdgeI path=\"%s:Dendrite-%s:Axon\">\n",sSink.c_str(),sOutput.c_str());   // Connect output to sink.
     unsigned omega_sink = static_cast<unsigned>(1024*(100/(exp(1)*Si_p.dc))/time_scale);
     omega_sink = omega_sink > omega_max ? omega_max : (omega_sink < omega_min ? omega_min : omega_sink);
     NeO_p.PSPrnt(fp,time_scale,omega_sink);                                                     // synaptic parameters
     fprintf(fp,"      </EdgeI>\n");                                                             // end connection
  }
  else if (monitored_neurons.find(N) != monitored_neurons.end())
     fprintf(fp,"      <EdgeI path=\":SpikeReceiver-%s:Axon_Sup\"/>\n",N->Name().c_str());       // recorder connection (still inefficient)
  WALKVECTOR(string,vNk,j)
  {
    // walk the edges
    fprintf(fp,"      <EdgeI path=\"%s:Dendrite-%s:Axon\">\n",(*j).c_str(),N->Name().c_str());   // Connection, remembering target is first.
    unsigned omega_tgt = static_cast<unsigned>(1024*(100/(exp(1)*Ne_p.dc))/time_scale);
    omega_tgt = omega_tgt > omega_max ? omega_max : (omega_tgt < omega_min ? omega_min : omega_tgt);
    if (N->param == "px")
       So_p.PSPrnt(fp,time_scale,omega_tgt);                                                     // parameters only if source is the output. May never apply.
    else Ne_p.PSPrnt(fp,time_scale,omega_tgt);                                                   // synaptic parameters
    fprintf(fp,"      </EdgeI>\n");                                                              // end connection
  }
}

                                                                                            // Connect source device
unsigned omega_in = static_cast<unsigned>(1024*(100/(exp(1)*Ne_p.dc))/time_scale);          // Find input neuron's natural frequency
omega_in = omega_in > omega_max ? omega_max : (omega_in < omega_min ? omega_min : omega_in); 
fprintf(fp,"      <EdgeI path=\":SpikeReceiver-%s:Axon_Sup\"/>\n",sSource.c_str());         // Connect source to supervisor.
fprintf(fp,"      <EdgeI path=\"%s:Dendrite-%s:Axon\">\n",sInput.c_str(),sSource.c_str());  // Connect source to input.
So_p.PSPrnt(fp,time_scale,omega_in);                                                        // synaptic parameters
fprintf(fp,"      </EdgeI>\n");                                                             // end connection
fprintf(fp,"      <EdgeI path=\":SpikeReceiver-%s:Axon_Sup\"/>\n",sSink.c_str());           // Connect sink to supervisor.
fprintf(fp,"    </EdgeInstances>\n");                                                       // end edge instance section
fprintf(fp,"  </GraphInstance>\n");                                                         // end graph instance
fprintf(fp,"</Graphs>\n");                                                                  // end definition
fclose(fp);                            // End of circuit file
t0 = mTimer(t0);
printf("...POETS circuit description file built in %ld wall ms]\n\n",t0);
}

//------------------------------------------------------------------------------

void WriteUIF(string ename,unsigned R)
{
long t0 = mTimer();
printf("\n[Writing Loader103 circuit description files...\n");
FILE * fp = fopen(ename.c_str(),"w");
if (fp == NULL)
{
   printf("Couldn't open output file %s: Loader103 file not generated\n", ename.c_str());
   return;
}


fprintf(fp,"[Circuit(\"%s\")]\n",ename.c_str());
fprintf(fp,"*pin : Sy1(\"\",\"\")\n");
fprintf(fp,"#pin : Sy1()\n");

So_p.UPrnt(fp);
Si_p.UPrnt(fp);
fprintf(fp,"%s : %s = %s\n",sSource.c_str(),So_p.sPrnt().c_str(),sInput.c_str());
fprintf(fp,"%s : %s \n",sSink.c_str(),Si_p.sPrnt().c_str());

Ne_p.name = "p0";                      // Default model name
Ne_p.th = 1.0;                         // Reset the threshold
Ne_p.UPrnt(fp);                        // Write to UIF as default model
char sp0[64];                          // Infinitely long string buffer
                                       // Modify threshold for the output neuron
sprintf(sp0," p0(,,%e) = %s",0.95*Ne_p.h*R,sSink.c_str());

WALKPDIGRAPHNODES(string,N_t *,int,int,int,int,G,i) {
  N_t * N = G.NodeData(i);             // Walk the devices
  string sType;
  char eq('=');
  if (N->Name()==sOutput) {            // Special case for the output device
    sType = sp0;
    eq=' ';
  }
  fprintf(fp,"%s :%s%c ",N->Name().c_str(),sType.c_str(),eq);  // Driving node....
  vector<string> vNk;
  G.DrivenNodes(N->Name(),vNk);        // Extract driven nodes
  WALKVECTOR(string,vNk,j) {
    char c = (*j)==vNk.back()?' ':',';
    fprintf(fp,"%s%c ",(*j).c_str(),c);
  }
  fprintf(fp,"\n");
}

fclose(fp);
t0 = mTimer(t0);
printf("...Loader103 circuit description files built in %ld wall ms]\n\n",t0);
}

//------------------------------------------------------------------------------
