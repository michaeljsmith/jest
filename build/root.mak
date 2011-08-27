.PHONY: default clean run

make_extension=mak
out_ext=out.$(make_extension)
dep_ext=dep.$(make_extension)
marker_ext=marker
dirmarker_ext=dirmarker
src_dir=.
obj_dir=obj
root_out=$(obj_dir)/.$(out_ext)

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

$(src_dir)/%: $(obj_dir)/%.input.jest.evaluated | $(obj_dir)/%$(dirmarker_ext)
	cp $< $@

$(obj_dir)/%.jest.evaluated: $(obj_dir)/%.jest.preprocessed | $(obj_dir)/%$(dirmarker_ext)
	cp $< $@

$(obj_dir)/%.jest.preprocessed: $(obj_dir)/%.jest.stripped | $(obj_dir)/%$(dirmarker_ext)
	cp $< $@

$(obj_dir)/%.jest.stripped: $(src_dir)/%.jest
	cp $< $@

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
