#pragma once

#include <array>

namespace spg
{
template <class Type, unsigned Depth>
class FrameMachine : std::array<Type, Depth>
{
public:
  using std::array<Type, Depth>::array;

  using std::array<Type, Depth>::operator[];

  Type& get() { return operator[](m_index); }

  const Type& get() const { return operator[](m_index); }

  Type& next() { return operator[]((m_index + 1) % Depth); }

  const Type& next() const { return operator[]((m_index + 1) % Depth); }

  Type& prev() { return operator[]((m_index + Depth - 1) % Depth); }

  const Type& prev() const { return operator[]((m_index + Depth - 1) % Depth); }

  void flip() { m_index = (m_index + 1) % Depth; }

private:
  unsigned m_index = 0;
};
}
