BINS = sysmenu
LIBS = libsysmenu.so
PKGS = gtkmm-4.0 gtk4-layer-shell-0
SRCS = $(filter-out src/main.cpp, $(wildcard src/*.cpp))
OBJS = $(SRCS:.cpp=.o)

PREFIX ?= /usr/local
BINDIR ?= $(PREFIX)/bin
LIBDIR ?= $(PREFIX)/lib
DATADIR ?= $(PREFIX)/share

CXXFLAGS += -Oz -s -Wall -flto -fno-exceptions -fPIC
LDFLAGS += -Wl,--as-needed,-z,now,-z,pack-relative-relocs

CXXFLAGS += $(shell pkg-config --cflags $(PKGS))
LDFLAGS += $(shell pkg-config --libs $(PKGS))

JOB_COUNT := $(EXEC) $(LIB) $(OBJS) src/git_info.hpp
JOBS_DONE := $(shell ls -l $(JOB_COUNT) 2> /dev/null | wc -l)

define progress
	$(eval JOBS_DONE := $(shell echo $$(($(JOBS_DONE) + 1))))
	@printf "[$(JOBS_DONE)/$(shell echo $(JOB_COUNT) | wc -w)] %s %s\n" $(1) $(2)
endef

all: $(BINS) $(LIBS)

install: $(all)
	@echo "Installing..."
	@install -D -t $(DESTDIR)$(BINDIR) $(BINS)
	@install -D -t $(DESTDIR)$(LIBDIR) $(LIBS)

clean:
	@echo "Cleaning up"
	@rm $(BINS) $(LIBS) $(SRCS:.cpp=.o) src/git_info.hpp

$(BINS): src/git_info.hpp src/main.cpp src/config_parser.o
	$(call progress, Linking $@)
	@$(CXX) -o $(BINS) \
	src/main.cpp \
	src/config_parser.o \
	$(CXXFLAGS) \
	$(shell pkg-config --libs gtkmm-4.0 gtk4-layer-shell-0)

$(LIBS): $(OBJS)
	$(call progress, Linking $@)
	@$(CXX) -o $(LIBS) \
	$(OBJS) \
	$(CXXFLAGS) \
	$(LDFLAGS) \
	-shared

%.o: %.cpp
	$(call progress, Compiling $@)
	@$(CXX) $(CFLAGS) -c $< -o $@ \
	$(CXXFLAGS)

src/git_info.hpp:
	$(call progress, Creating $@)
	@commit_hash=$$(git rev-parse HEAD); \
	commit_date=$$(git show -s --format=%cd --date=short $$commit_hash); \
	commit_message=$$(git show -s --format=%s $$commit_hash); \
	echo "#define GIT_COMMIT_MESSAGE \"$$commit_message\"" > src/git_info.hpp; \
	echo "#define GIT_COMMIT_DATE \"$$commit_date\"" >> src/git_info.hpp
