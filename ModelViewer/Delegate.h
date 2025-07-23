#pragma once

#ifndef DELEGATE_H
#define DELEGATE_H

#include <vector>
#include <functional>
#include <algorithm>

template <typename Signature>
class Delegate;

template <typename Ret, typename... Args>
class Delegate<Ret(Args...)>
{
public:
	void Bind(const std::function<Ret(Args...)>& Callback)
	{
		m_Callbacks.push_back(Callback);
	}

	void Broadcast(Args... args)
	{
		for (const std::function<Ret(Args...)>& Callback : m_Callbacks)
		{
			Callback(args...);
		}
	}

private:
	std::vector<std::function<Ret(Args...)>> m_Callbacks;

};

#endif
