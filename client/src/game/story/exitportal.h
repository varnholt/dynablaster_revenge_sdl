#pragma once

#include <cstdint>
#include <vector>

//! the story mode's exit, a stone ring set into the floor: an iris seals it while enemies are
//! left, once the stage is cleared it opens onto a swirling funnel sinking below the floor
class ExitPortal
{
public:
   ExitPortal();
   ~ExitPortal();

   ExitPortal(const ExitPortal&) = delete;
   ExitPortal& operator=(const ExitPortal&) = delete;

   //! the exit has been revealed on tile x, y
   void show(int32_t x, int32_t y);
   void hide();

   [[nodiscard]] bool isShown() const;
   [[nodiscard]] bool isAt(int32_t x, int32_t y) const;
   [[nodiscard]] int32_t getX() const;
   [[nodiscard]] int32_t getY() const;

   //! \return \c true if a shown portal just opened
   bool setOpen(bool open);

   //! \param dt game ticks since the last frame (62.5 per second)
   void render(float dt);

private:
   //! the pieces, matching the shader's "part" uniform
   enum class Part : int32_t
   {
      Rim,
      Funnel,
      Lid,
      Glow,
      DepthPunch
   };

   struct Range
   {
      int32_t _first = 0;
      int32_t _count = 0;
   };

   void buildGeometry();
   void draw(Part part);

   bool _shown = false;
   bool _open = false;
   int32_t _x = 0;
   int32_t _y = 0;

   //! eases towards _open
   float _openness = 0.0f;

   std::vector<Range> _ranges;

   uint32_t _shader = 0;
   int32_t _part_param = -1;
   int32_t _time_param = -1;
   int32_t _openness_param = -1;
   int32_t _aperture_param = -1;
   uint32_t _vertex_buffer = 0;
};
