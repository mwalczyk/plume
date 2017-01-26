#pragma once

class Noncopyable
{
protected:

	Noncopyable() = default;
	~Noncopyable() = default;
	Noncopyable(const Noncopyable &tOther) = delete;
	Noncopyable& operator=(const Noncopyable &tOther) = delete;
};