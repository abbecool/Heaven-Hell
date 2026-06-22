#include "TestSupport.hpp"
#include "assets/SpriteDefinition.hpp"

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

void testTextureSizeConstructor()
{
    const SpriteDefinition sprite{
        "walk",
        TextureHandle{"character_sheet"},
        8,
        4,
        2,
        4,
        TextureSize{128, 64}
    };

    require(sprite.name() == "walk", "sprite name was incorrect");
    require(sprite.texture().name == "character_sheet", "texture handle was incorrect");
    require(sprite.frameCount() == 8, "frame count was incorrect");
    require(sprite.frameDuration() == 4, "frame duration was incorrect");
    require(sprite.cols() == 4, "column count was incorrect");
    require(sprite.isAnimated(), "animated sprite was marked static");
    require(nearlyEqual(sprite.frameSize().x, 32.0f), "frame width was incorrect");
    require(nearlyEqual(sprite.frameSize().y, 32.0f), "frame height was incorrect");
    requireRect(sprite.firstFrame(), 0.0f, 0.0f, 32.0f, 32.0f);
    requireRect(sprite.frameRect(3, 1), 96.0f, 32.0f, 32.0f, 32.0f);
}

void testSourceRegionConstructor()
{
    const SpriteDefinition sprite{
        "atlas_walk",
        TextureHandle{"atlas"},
        6,
        2,
        2,
        3,
        RectF{16.0f, 32.0f, 96.0f, 64.0f}
    };

    requireRect(sprite.sourceRegion(), 16.0f, 32.0f, 96.0f, 64.0f);
    require(nearlyEqual(sprite.frameSize().x, 32.0f), "atlas frame width was incorrect");
    require(nearlyEqual(sprite.frameSize().y, 32.0f), "atlas frame height was incorrect");
    requireRect(sprite.frameRect(2, 1), 80.0f, 64.0f, 32.0f, 32.0f);
}

void testMinimumValues()
{
    const SpriteDefinition sprite{
        "static",
        TextureHandle{"single"},
        0,
        0,
        0,
        0,
        TextureSize{64, 32}
    };

    require(sprite.frameCount() == 1, "zero frame count was not clamped");
    require(sprite.frameDuration() == 1, "zero frame duration was not clamped");
    require(sprite.cols() == 1, "zero columns were not clamped");
    require(!sprite.isAnimated(), "single-frame sprite was marked animated");
    requireRect(sprite.firstFrame(), 0.0f, 0.0f, 64.0f, 32.0f);
}

constexpr std::array Tests = {
    TestSupport::TestCase{"texture_size_constructor", testTextureSizeConstructor},
    TestSupport::TestCase{"source_region_constructor", testSourceRegionConstructor},
    TestSupport::TestCase{"minimum_values", testMinimumValues}
};

} // namespace

int main(int argc, char* argv[])
{
    return TestSupport::runNamedTest(argc, argv, Tests);
}
