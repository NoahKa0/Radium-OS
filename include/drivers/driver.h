#ifndef __SYS__DRIVERS__DRIVER_H
#define __SYS__DRIVERS__DRIVER_H

  namespace sys {
    namespace hardware {
      class PeripheralComponentDeviceDescriptor;
      class InterruptManager;
    }
    namespace drivers {
      class Driver {
      public:
        Driver();
        ~Driver();
        
        virtual void activate();
        virtual int reset();
        virtual void deactivate();

        static Driver* getDriver(hardware::PeripheralComponentDeviceDescriptor* device, hardware::InterruptManager* interruptManager);
      };
      
      class DriverManager {
      private:
        Driver* drivers[255];
        int numDrivers;
      public:
        DriverManager();
        void addDriver(Driver*);
        void activateAll();
      };
    }
  }
    
#endif
