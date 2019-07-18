#ifndef __SYS__GDT_H
#define __SYS__GDT_H

    #include <common/types.h>
    namespace sys {
        class GlobalDescriptorTable
        {
            public:

                class SegmentDescriptor
                {
                    private:
                        sys::common::uint16_t limit_lo;
                        sys::common::uint16_t base_lo;
                        sys::common::uint8_t base_hi;
                        sys::common::uint8_t type;
                        sys::common::uint8_t limit_hi;
                        sys::common::uint8_t base_vhi;

                    public:
                        SegmentDescriptor(sys::common::uint32_t base, sys::common::uint32_t limit, sys::common::uint8_t type);
                        sys::common::uint32_t Base();
                        sys::common::uint32_t Limit();
                } __attribute__((packed));

            private:
                SegmentDescriptor nullSegmentSelector;
                SegmentDescriptor unusedSegmentSelector;
                SegmentDescriptor codeSegmentSelector;
                SegmentDescriptor dataSegmentSelector;

            public:

                GlobalDescriptorTable();
                ~GlobalDescriptorTable();

                sys::common::uint16_t CodeSegmentSelector();
                sys::common::uint16_t DataSegmentSelector();
        };
    }

#endif
