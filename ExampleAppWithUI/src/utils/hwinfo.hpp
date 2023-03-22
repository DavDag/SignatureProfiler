#pragma once

namespace hwinfo {

    void init();
    void exit();

    namespace extra {
        int pid();
        int runningCoreInd();
    }

    namespace opengl {
        const char* version();
    }
    
    namespace gpu {
        const char* vendor();
        const char* renderer();
        bool isIntel();
        bool isNVidia();
        bool isAMD();
        double usage();
        double usageMb();
        double availableMb();
        double physicalTotMb();
    }

    namespace cpu {
        unsigned int threadCount();
        const char* vendor();
        const char* brand();
        double usage(double cores[]);
    }

    namespace mem {
        double usageMb();
        double availableMb();
        double physicalTotMb();
    }

    namespace deps {
        const char* glfwVersion();
        const char* glewVersion();
        const char* imguiVersion();
        const char* glmVersion();
    }

}
