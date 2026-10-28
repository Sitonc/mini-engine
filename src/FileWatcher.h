#pragma once
#include <filesystem>
#include <system_error>
#include <utility>

class FileWatcher
{
public:
	explicit FileWatcher(std::filesystem::path path)
		: m_path(std::move(path)) { m_last = Stamp(); }

	bool Changed()
	{
		const auto now = Stamp();
		if (now == m_last) return false;
		m_last = now;
		return true;
	}

private:
	std::filesystem::file_time_type Stamp() const
	{
		std::error_code ec;
		const auto t = std::filesystem::last_write_time(m_path, ec);
		return ec ? m_last : t;
	}

	std::filesystem::path m_path;
	std::filesystem::file_time_type m_last{};
};