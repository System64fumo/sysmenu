EXEC = sysmenu
PKGS = gtkmm-4.0 gtk4-layer-shell-0
SRCS +=	$(wildcard src/*.cpp)
OBJS = $(SRCS:.cpp=.o)
DESTDIR = $(HOME)/.local

CXXFLAGS = -march=native -mtune=native -Os -s -Wall -flto=auto -fno-exceptions
CXXFLAGS += $(shell pkg-config --cflags $(PKGS))
LDFLAGS += $(shell pkg-config --libs $(PKGS)) -Wl,--gc-sections

$(EXEC): src/git_info.hpp $(OBJS)
	$(CXX) -o $(EXEC) $(OBJS) \
	$(LDFLAGS) \
	$(CXXFLAGS)

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@ \
	$(CXXFLAGS)

src/git_info.hpp:
	@commit_hash=$$(git rev-parse HEAD); \
	commit_date=$$(git show -s --format=%cd --date=short $$commit_hash); \
	commit_message=$$(git show -s --format=%s $$commit_hash); \
	echo "#define GIT_COMMIT_MESSAGE \"$$commit_message\"" > src/git_info.hpp; \
	echo "#define GIT_COMMIT_DATE \"$$commit_date\"" >> src/git_info.hpp

install: $(EXEC)
	mkdir -p $(DESTDIR)/bin
	install $(EXEC) $(DESTDIR)/bin/$(EXEC)

clean:
	rm $(EXEC) $(SRCS:.cpp=.o) src/git_info.hpp
