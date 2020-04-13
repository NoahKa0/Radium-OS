#ifndef __SYS__FILESYSTEM__FILESYSTEM_H
#define __SYS__FILESYSTEM__FILESYSTEM_H

  #include <common/types.h>
  #include <common/string.h>

  namespace sys {
    namespace filesystem {
      class FileSystem;

      class File {
      protected:
        FileSystem* fileSystem;
      public:
        File(FileSystem* fileSystem);
        virtual ~File();
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

        FileSystem* getFileSystem();
      };
      class FileSystem {
      public:
        FileSystem();
        virtual ~FileSystem();

        virtual File* getRoot();
        virtual common::String* getName();
        virtual common::String* getType();
      };
    }
  }

#endif