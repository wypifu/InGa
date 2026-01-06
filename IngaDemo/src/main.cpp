#include <InGa/core/Window.h>
#include <InGa/gfx/RenderDevice.h>
#include <InGa/gfx/Context.h>
#include <InGa/core/log.h>
#include <InGa/InGa.h>

using namespace Inga;

I32 main(I32 argc, char** argv)
{
    // 0. START THE ENGINE CORE
    // 64 groups, 16MB default page size
    Inga::EngineConfig conf = Inga::getDefaultEngineConfig();
    INGA_BEGIN(conf)

    INGA_LOG(eINFO, "CORE", "InGa Engine Core Started.");
	CRenderDevice sharedGPU;
    SContextConf config1 = { "Primary Window", 1280, 720 };
    CContext ctx1;
    ctx1.initialize(&sharedGPU, config1);
    /*
    SContextConf config2 = { "Secondary Window", 800, 600 };
    CContext ctx2;
    // The second context just attaches to the already-live hardware
    ctx2.initialize(&sharedGPU, config2);
    */
    U32 frameCount = 0;

    INGA_LOG(eINFO, "DEMO", "Shutdown complete. Goodbye.");
    INGA_END()
    return 0;
}
