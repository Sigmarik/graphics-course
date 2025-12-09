#pragma once

#include <array>

template <class Type, unsigned Depth>
class FrameMachine : std::array<Type, Depth>
{
public:
  using std::array<Type, Depth>::array;

  using std::array<Type, Depth>::operator[];

  Type& get() { return operator[](m_index); }

  const Type& get() const { return operator[](m_index); }

  void flip() { m_index = (m_index + 1) % Depth; }

private:
  unsigned m_index = 0;
};
