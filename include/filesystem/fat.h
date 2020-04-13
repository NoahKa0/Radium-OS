#ifndef __SYS__FILESYSTEM__FAT_H
#define __SYS__FILESYSTEM__FAT_H

  #include <common/types.h>
  #include <common/string.h>
  #include <filesystem/partition/partition.h>
  #include <filesystem/filesystem.h>

  namespace sys {
    namespace filesystem {

      struct fatBPB {
        common::uint8_t jump[3];
        common::uint8_t softname[8];
        common::uint16_t byesPerSector;
        common::uint8_t sectorsPerCluster;
        common::uint16_t reservedSectors;
        
        common::uint8_t fatCopies;
        common::uint16_t rootDirEntries;
        common::uint16_t totalSectors;
        common::uint8_t mediaType;
        common::uint16_t fatSectorCount;
        common::uint16_t sectorsPerTrack;
        common::uint16_t headCount;
        common::uint32_t hiddenSectors;
        common::uint32_t totalSectorCount;

        common::uint32_t tableSize;
        common::uint16_t extFlags;
        common::uint16_t fatVersion;
        common::uint32_t rootCluster;
        common::uint16_t fatInfo;
        common::uint16_t backupSector;
        common::uint32_t reserved[3];
        common::uint8_t driveNumber;
        common::uint8_t reserved1;
        common::uint8_t signature;
        common::uint32_t volumeId;
        common::uint8_t volumeLabel[11];
        common::uint8_t fatTypeLabel[8];
      } __attribute__((packed));

      struct fatDirectoryEntry {
        common::uint8_t name[8];
        common::uint8_t extension[3];
        common::uint8_t attributes;
        common::uint8_t reserved;
        common::uint8_t createdTimeTenth;

        common::uint16_t createdTime;
        common::uint16_t createdDate;
        common::uint16_t accessedTime;

        common::uint16_t firstClusterHi;
        common::uint16_t writeTime;
        common::uint16_t writeDate;
        common::uint16_t firstClusterLow;
        common::uint32_t size;
      } __attribute__((packed));

      struct FATInfo {
        common::uint32_t signature1;
        common::uint8_t reserved[480];
        common::uint32_t signature2;
        common::uint32_t freeClusterCount;
        common::uint32_t startAvailableClusters;
      } __attribute__((packed));

      class Fat;
      class FatFile: public File {
      friend class Fat;
      private:
        Fat* fat;
        common::uint32_t parentCluster;
        bool directory;
        common::uint32_t sectorOffset;
        common::uint32_t firstFileCluster;
        common::uint32_t nextFileCluster;
        common::uint32_t lastCluster;
        common::int32_t size;
        common::int32_t readPosition;
        common::uint8_t* buffer;
        common::String* filename;

        FatFile(Fat* fat, bool isFolder, common::uint32_t firstFileCluster, common::uint32_t parentCluster, common::String* filename, common::uint32_t size = 0);
        void loadNextSector();
        void reset();

        void writeBuffer();
        bool updateChild(common::uint32_t childCluster, common::uint32_t size, common::String* name = 0);
        bool deleteChild(common::uint32_t childCluster);

        static common::String* getRealFilename(common::String* filename);
      public:
        ~FatFile();

        virtual bool isFolder();
        virtual File* getParent();
        virtual File* getChild(common::uint32_t file);
        virtual File* getChildByName(common::String* file);
        virtual common::uint32_t numChildren();
        virtual bool mkFile(common::String* filename, bool folder = false);

        virtual bool append(common::uint8_t* data, common::uint32_t length);

        virtual int hasNext();
        virtual common::uint8_t nextByte();
        virtual void read(common::uint8_t* buffer, common::uint32_t length);
        virtual common::String* getFilename();
        virtual bool remove();
        virtual bool rename(common::String* name);
      };

      class Fat: public FileSystem {
      friend class FatFile;
      private:
        fatBPB* bpb;
        FATInfo* info;
        partition::Partition* partition;

        common::uint32_t fatStart;
        common::uint32_t dataStart;
        common::uint32_t rootStart;
        common::uint8_t sectorsPerCluster;

        common::uint32_t chain(common::uint32_t lastCluster, bool force = false);
        bool deleteChain(common::uint32_t startCluster);
      public:
        Fat(partition::Partition* partition);
        ~Fat();
        virtual File* getRoot();
        virtual common::String* getName();
        virtual common::String* getType();

        static bool isFat(partition::Partition* partition);
        static void readBPB(partition::Partition* partition);
      };

    }
  }

#endif