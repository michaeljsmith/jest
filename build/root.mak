.PHONY: default clean run

make_extension=mak
dep_ext=dep.$(make_extension)
marker_ext=marker
src_dir=.
obj_dir=obj
dep_dir=$(obj_dir)/dep
root_dep=$(dep_dir)/.$(dep_ext)

-include $(root_dep)

.PHONY: default clean
default: $(outputs)

clean:
	rm -rf $(obj_dir)

$(dep_dir)/%$(dep_ext): $(src_dir)/% $(dep_dir)/%$(marker_ext)
	$(output_directory_fragment)

%/.$(marker_ext):
	@mkdir -p $(@D)
	@touch $@
.PRECIOUS: %/.$(marker_ext)

# Generate dependency makefile fragment for a directory.
define output_directory_fragment
@output_path=$@; \
directory=$(module_source_dir)/$*; \
object_directory=$(module_obj_path)/$*; \
directory_marker_path=$$directory/.$(dirmarker_extension); \
echo "" > $$output_path; \
if [ -f $$directory_marker_path ]; then \
	exit 0; \
fi; \
dependency_directory=$$(dirname $$output_path); \
entries=$$(ls -p $$directory | grep "^[A-Za-z]"); \
for entry in $$entries; do \
	echo "-include $$dependency_directory/$$entry.$(dependency_extension)" >> $$output_path; \
done; \
echo "" >> $$output_path;
endef
