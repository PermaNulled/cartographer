#pragma once

std::string IOCTLSocket_cmd_string(long cmd);
std::string sockOpt_string(int optName);

bool Ipv4AddressIsReservedOrLocalhost(const IN_ADDR ipv4Addr);

struct NetElement
{
	void* data;
	NetElement* previous;
	NetElement* next;
};

struct NetLinkedList
{
	NetElement* m_first;
	NetElement* m_last;

	size_t m_count;

	NetLinkedList()
	{
		m_first = nullptr;
		m_last = nullptr;
		m_count = 0;
	}

public:
	NetElement* first() const
	{
		return m_first;
	}

	NetElement* last() const
	{
		return m_last;
	}

	NetElement* insert(void* data)
	{
		NetElement* element = (NetElement*)malloc(sizeof(NetElement));

		element->next = nullptr;
		element->data = data;

		if (m_first == nullptr)
		{
			m_first = element;
			m_last = element;
			element->previous = nullptr;
		}
		else
		{
			m_last->next = element;
			element->previous = m_last;
			m_last = element;
		}

		m_count++;
		return element;
	}

	void remove(NetElement* element)
	{
		NetElement* cur = m_first;
		while (cur)
		{
			if (cur == element)
			{
				if (cur->previous != nullptr)
				{
					cur->previous->next = cur->next;
				}
				else
				{
					m_first = cur->next;
				}

				if (cur->next != nullptr)
				{
					cur->next->previous = cur->previous;
				}
				else
				{
					m_last = cur->previous;
				}

				m_count--;
				free(cur);
				break;
			}

			cur = cur->next;
		}
	}

	void dispose()
	{
		NetElement* cur = m_last;
		while (cur)
		{
			NetElement* prev = cur->previous;
			free(cur->data);
			free(cur);
			cur = prev;
			m_count--;
		}

		m_first = nullptr;
		m_last = nullptr;
	}
};