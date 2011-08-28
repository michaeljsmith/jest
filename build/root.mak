.PHONY: default clean run

make_extension=mak
out_ext=out.$(make_extension)
dep_ext=dep.$(make_extension)
marker_ext=marker
dirmarker_ext=dirmarker
src_dir=.
obj_dir=obj
root_out=$(obj_dir)/.$(out_ext)

CXX=g++
CXXFLAGS=-Wall -Wextra -Werror

.PHONY: default clean all_outputs
default: all_outputs

outputs=
-include $(root_out)

all_outputs: $(outputs)

clean:
	rm -rf $(outputs) $(obj_dir)

$(obj_dir)/%$(out_ext): $(src_dir)/% $(obj_dir)/%$(dirmarker_ext)
	$(output_directory_fragment)

$(obj_dir)/%.jest.$(dep_ext): $(src_dir)/%.jest
	bash build/scripts/dep_mak.jest.sh $(src_dir)/$*.jest $(obj_dir)/$*.jest > $@

$(obj_dir)/%.input.jest.$(out_ext): $(src_dir)/%.input.jest
	bash build/scripts/out_mak.jest.sh $(src_dir)/$* $(obj_dir)/$*.input.jest > $@

$(src_dir)/%: $(obj_dir)/%.input.jest.evaluated
	cp $< $@

$(obj_dir)/%.jest.evaluated: $(obj_dir)/%.jest.evaluated.gen
	$< > $@

$(obj_dir)/%.jest.evaluated.gen: $(obj_dir)/%.jest.preprocessed
	$(CXX) $(CXXFLAGS) -I. -o $@ -D PREPROCESSED_FILE=\"$(obj_dir)/$*.jest.preprocessed\" build/fragments/evaluate.cpp

$(obj_dir)/%.jest.preprocessed: $(obj_dir)/%.jest.preprocessed.gen
	$< > $@
.PRECIOUS: $(obj_dir)/%.jest.preprocessed

$(obj_dir)/%.jest.preprocessed.gen: $(obj_dir)/%.jest.predeclared $(obj_dir)/%.jest.collated build/fragments/preprocess.cpp
	$(CXX) $(CXXFLAGS) -I. -o $@ -D PREDECLARED_FILE=\"$(obj_dir)/$*.jest.predeclared\" -D COLLATED_FILE=\"$(obj_dir)/$*.jest.collated\" build/fragments/preprocess.cpp

$(obj_dir)/%.jest.predeclared: $(obj_dir)/%.jest.collated
	grep -o "[A-Za-z_]\+" $< | sort | uniq | sed -e 's/^\(.*\)$$/JEST_DEFINE(\1)/' > $@
.PRECIOUS: $(obj_dir)/%.jest.predeclared

$(obj_dir)/%.jest.collated: $(src_dir)/%.jest
	sed -nf build/scripts/import.sed $< | sed 'N;N;s/\n//' | sed -f - $< >$@
.PRECIOUS: $(obj_dir)/%.jest.collated

%/.$(dirmarker_ext):
	@mkdir -p $(@D)
	@touch $@
.PRECIOUS: %/.$(dirmarker_ext)

%.$(marker_ext):
	@touch $@
.PRECIOUS: %.$(marker_ext)

# Generate dependency makefile fragment for a directory.
define output_directory_fragment
@output_path=$@; \
directory=$(src_dir)/$*; \
object_directory=$(obj_dir)/$*; \
directory_marker_path=$$directory/.$(dirmarker_ext); \
echo "" > $$output_path; \
if [ -f $$directory_marker_path ]; then \
	exit 0; \
fi; \
dependency_directory=$$(dirname $$output_path); \
dirs=$$(ls -p $$directory | grep "^[A-Za-z].*/$$"); \
for entry in $$dirs; do \
	echo "-include $$dependency_directory/$$entry.$(out_ext)" >> $$output_path; \
done; \
inputs=$$(ls -p $$directory | grep "^[A-Za-z].*.input.jest$$"); \
for entry in $$inputs; do \
	echo "-include $$dependency_directory/$$entry.$(out_ext)" >> $$output_path; \
done; \
echo "" >> $$output_path;
endef
