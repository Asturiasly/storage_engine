#include <filesystem>


namespace StorageEngine 
{
    class DiskManager
    { 
        public:
            explicit DiskManager(std::filesystem::path path) noexcept : path_(path) {}

            //deleted default constructors
            //Only 1 DiskManager is allowed
            //Probably movable, but now is not necessary

            DiskManager() = delete;
            DiskManager(const DiskManager&) = delete;
            DiskManager(DiskManager&&) = delete;
            DiskManager operator=(const DiskManager&) = delete;

        private:
            std::filesystem::path path_;
    };
} //StorageEngine