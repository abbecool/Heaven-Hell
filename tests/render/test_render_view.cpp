#include "TestSupport.hpp"
#include "render/RenderTypes.hpp"

#include <array>
#include <cmath>

namespace {

using TestSupport::require;

bool nearlyEqual(float lhs, float rhs, float epsilon = 0.0001f)
{
    return std::fabs(lhs - rhs) <= epsilon;
}

void requireRect(const RectF& rect, float x, float y, float width, float height)
{
    require(nearlyEqual(rect.x, x), "unexpected rectangle x");
    require(nearlyEqual(rect.y, y), "unexpected rectangle y");
    require(nearlyEqual(rect.w, width), "unexpected rectangle width");
    require(nearlyEqual(rect.h, height), "unexpected rectangle height");
}

void testPointConversion()
{
    const RenderView view{100.0f, 50.0f, 2.0f, 320.0f, 180.0f};

    require(nearlyEqual(view.worldToScreenX(100.0f), 320.0f), "camera x did not map to origin");
    require(nearlyEqual(view.worldToScreenY(50.0f), 180.0f), "camera y did not map to origin");
    require(nearlyEqual(view.worldToScreenX(112.5f), 345.0f), "world x conversion was incorrect");
    require(nearlyEqual(view.worldToScreenY(65.0f), 210.0f), "world y conversion was incorrect");
}

void testRectConversion()
{
    const RenderView view{10.0f, 20.0f, 1.5f, 100.0f, 80.0f};
    const RectF screenRect = view.worldToScreen(RectF{14.0f, 24.0f, 8.0f, 6.0f});

    requireRect(screenRect, 106.0f, 86.0f, 12.0f, 9.0f);
}

void testCameraZoomOrigin()
{
    const int virtualWidth = 640;
    const int virtualHeight = 360;
    const int windowScale = 3;
    const int cameraZoom = 1;
    const float totalZoom = static_cast<float>(windowScale - cameraZoom);
    const RenderView view{
        80.0f,
        45.0f,
        totalZoom,
        static_cast<float>(virtualWidth) * cameraZoom / 2.0f,
        static_cast<float>(virtualHeight) * cameraZoom / 2.0f
    };

    require(nearlyEqual(view.worldToScreenX(80.0f), 320.0f), "legacy zoom origin changed on x");
    require(nearlyEqual(view.worldToScreenY(45.0f), 180.0f), "legacy zoom origin changed on y");
}

constexpr std::array Tests = {
    TestSupport::TestCase{"point_conversion", testPointConversion},
    TestSupport::TestCase{"rect_conversion", testRectConversion},
    TestSupport::TestCase{"camera_zoom_origin", testCameraZoomOrigin}
};

} // namespace

int main(int argc, char* argv[])
{
    return TestSupport::runNamedTest(argc, argv, Tests);
}
