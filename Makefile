# This file is part of OpenPanel - The Open Source Control Panel
# OpenPanel is free software: you can redistribute it and/or modify it 
# under the terms of the GNU General Public License as published by the Free 
# Software Foundation, using version 3 of the License.
#
# Please note that use of the OpenPanel trademark may be subject to additional 
# restrictions. For more information, please visit the Legal Information 
# section of the OpenPanel website on http://www.openpanel.com/


include makeinclude

OBJ	= main.o version.o

all: module.xml postfixcouriermodule.exe masterconf
	grace mkapp postfixcouriermodule
	@mkdir -p tmp

module.xml: module.def
	mkmodulexml < module.def > module.xml

version.cpp:
	grace mkversion version.cpp

postfixcouriermodule.exe: $(OBJ)
	$(LD) $(LDFLAGS) -o postfixcouriermodule.exe $(OBJ) \
	/usr/lib/openpanel-core/libcoremodule.a $(LIBS)

masterconf: masterconf.o
	$(LD) $(LDFLAGS) -o masterconf masterconf.o $(LIBS)

clean:
	rm -f *.o *.exe
	rm -rf postfixcouriermodule.app
	rm -f postfixcouriermodule

SUFFIXES: .cpp .o
.cpp.o:
	$(CXX) $(CXXFLAGS) $(INCLUDES) -I../opencore/api/c++/include -c $<
