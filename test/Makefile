# Relative path
RELATIVE_PATH=..
APSSCHED=$(RELATIVE_PATH)/bin/apssched
EXAMPLES=$(RELATIVE_PATH)/examples
BASE=.:$(RELATIVE_PATH)/base:$(EXAMPLES)
FLAGS=-DCOT
CASES=simple-binding1 simple-binding2 simple-binding3

# Generate output and store it in a file
%.output: $(EXAMPLES)/%.aps
	$(APSSCHED) $(FLAGS) -p $(BASE) $(basename $<) > $@

# Copy actual output as expected
%.expected: %.output
	cp -f $< $@

# Compare actual output with expected
.PHONY: %.compare
%.compare: %.output
# maybe there is a better way without using basename ...
	diff $(basename $@).expected $<

# Generate and verify all outputs
.PHONY: all
all: $(addsuffix .compare,$(CASES))

# Regenerate expected output
.PHONY: build.expected
build.expected: $(addsuffix .expected,$(CASES))

.PHONY: clean.expected
clean.expected:
	-rm -f *.expected
