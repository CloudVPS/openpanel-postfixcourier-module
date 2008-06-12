include makeinclude

OBJ	= main.o version.o

all: module.xml postfixcouriermodule.exe masterconf
	mkapp postfixcouriermodule
	@mkdir -p tmp

module.xml: module.def
	mkmodulexml < module.def > module.xml

version.cpp:
	mkversion version.cpp

postfixcouriermodule.exe: $(OBJ)
	$(LD) $(LDFLAGS) -o postfixcouriermodule.exe $(OBJ) \
	../opencore/api/c++/lib/libcoremodule.a $(LIBS)

masterconf: masterconf.o
	$(LD) $(LDFLAGS) -o masterconf masterconf.o $(LIBS)

clean:
	rm -f *.o *.exe
	rm -rf postfixcouriermodule.app
	rm -f postfixcouriermodule

SUFFIXES: .cpp .o
.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) -I../opencore/api/c++/include -c $<
