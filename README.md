Storage engine for some database.

Build is simply organized via storage_engine/workflow/full_workflow.sh script which builds 6 different debug builds and executing tests automatically (requires gcc/clang compilers installed). Script must be located in /workflow directory.

./full_workflow.sh

args:
--fresh (full rebuild, non-incremental)

