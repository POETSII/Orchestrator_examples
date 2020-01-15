GCCDIR    = /usr
LIBDIR    = ../lib
INCDIR    = ../include
BUILDDIR  = build
BINDIR    = bin

CC        = $(GCCDIR)/bin/gcc
CXX       = $(GCCDIR)/bin/g++

CPPFLAGS += -std=c++98 -Wall -fPIC -pedantic -I $(INCDIR)

LDFLAGS += -L$(LIBDIR) 

GENERIC_SRC := flat.cpp rand.cpp lex.cpp dfprintf.cpp filename.cpp dumpchan.cpp

SYNFIRE_SRC := SynfireMainPOETS.cpp
SYNFIRE_SRC += $(GENERIC_SRC)
SYNFIRE_DIROBJ := $(patsubst %.cpp,%.o,$(SYNFIRE_SRC))
SYNFIRE_OBJ = $(addprefix $(BUILDDIR)/,$(notdir $(SYNFIRE_DIROBJ)))
# SYNFIRE_OBJ := $(SYNFIRE_SRC:%.cpp=$(BUILDDIR)/%.o)
SYNFIRE_TGT = $(BINDIR)/SynfireGenerator

all : $(SYNFIRE_TGT)

.PHONY : all clean

#$(BUILDDIR)/%.o: 

$(SYNFIRE_OBJ) : $(SYNFIRE_SRC)
	mkdir -p $(BUILDDIR)
	$(CXX) -c $(CPPFLAGS) -o $@ $(patsubst %.o,%.cpp,$(notdir $@)) 

$(SYNFIRE_TGT) : $(SYNFIRE_OBJ)
	mkdir -p $(BINDIR)
	$(CXX) $(CPPFLAGS) -o $@ $^ $(LDFLAGS) 

clean :
	rm -rf $(SYNFIRE_TGT) $(SYNFIRE_OBJ) $(SYNFIRE_DIROBJ) 



