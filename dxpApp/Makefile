TOP = ..
include $(TOP)/configure/CONFIG
DIRS += handelSrc
DIRS += src
DIRS += op
src_DEPEND_DIRS = handelSrc
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *db*))
DIRS := $(DIRS) $(filter-out $(DIRS), $(wildcard *Db*))
include $(TOP)/configure/RULES_DIRS

