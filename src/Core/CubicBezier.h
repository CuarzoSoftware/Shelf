#ifndef CUBICBEZIER_H
#define CUBICBEZIER_H

#include <CZ/Core/Cuarzo.h>
#include <cmath>

/**
 * @brief CSS / Core Animation style timing function: cubic-bezier(x1, y1, x2, y2).
 *
 * Maps a linear progress t in [0, 1] to the eased progress, solving x(s) = t for the curve
 * parameter s (Newton-Raphson with a bisection fallback) and returning y(s).
 */
class CubicBezier
{
public:
    constexpr CubicBezier(CZ::Float64 x1, CZ::Float64 y1, CZ::Float64 x2, CZ::Float64 y2) noexcept :
        m_cx(3.0 * x1), m_bx(3.0 * (x2 - x1) - 3.0 * x1), m_ax(1.0 - 3.0 * x1 - (3.0 * (x2 - x1) - 3.0 * x1)),
        m_cy(3.0 * y1), m_by(3.0 * (y2 - y1) - 3.0 * y1), m_ay(1.0 - 3.0 * y1 - (3.0 * (y2 - y1) - 3.0 * y1)) {}

    CZ::Float64 operator()(CZ::Float64 t) const noexcept
    {
        if (t <= 0.0) return 0.0;
        if (t >= 1.0) return 1.0;
        return sampleY(solveX(t));
    }

private:
    CZ::Float64 sampleX(CZ::Float64 s) const noexcept { return ((m_ax * s + m_bx) * s + m_cx) * s; }
    CZ::Float64 sampleY(CZ::Float64 s) const noexcept { return ((m_ay * s + m_by) * s + m_cy) * s; }
    CZ::Float64 sampleDX(CZ::Float64 s) const noexcept { return (3.0 * m_ax * s + 2.0 * m_bx) * s + m_cx; }

    CZ::Float64 solveX(CZ::Float64 x) const noexcept
    {
        CZ::Float64 s { x };

        for (int i = 0; i < 8; i++)
        {
            const CZ::Float64 err { sampleX(s) - x };
            if (std::fabs(err) < 1e-7) return s;
            const CZ::Float64 d { sampleDX(s) };
            if (std::fabs(d) < 1e-6) break;
            s -= err / d;
        }

        CZ::Float64 lo { 0.0 }, hi { 1.0 };
        s = x;

        while (lo < hi)
        {
            const CZ::Float64 v { sampleX(s) };
            if (std::fabs(v - x) < 1e-7) return s;
            if (x > v) lo = s; else hi = s;
            if (hi - lo < 1e-9) break;
            s = (lo + hi) * 0.5;
        }

        return s;
    }

    CZ::Float64 m_cx, m_bx, m_ax, m_cy, m_by, m_ay;
};

#endif // CUBICBEZIER_H
