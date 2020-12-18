#include "../Public/Lexical.h"
#include <map>
namespace PineApple::Lexical
{

	void MakeUpUnfaTable(Unfa::Table& table)
	{
		for(auto& ite : table.edges)
		{
			if(ite.type == Unfa::Table::EdgeType::Capture)
				ite.type = Unfa::Table::EdgeType::Epsilon;
		}
	}

	void Table::LexicalFilter(Unfa::Table const& table, std::vector<Unfa::Table::Edge>& edges)
	{
		using Edge = Unfa::Table::Edge;
		using EdgeType = Unfa::Table::EdgeType;
		std::optional<size_t> acception_jume_state;
		edges.erase(std::remove_if(edges.begin(), edges.end(), [&](Edge edge)
		{
			if (edge.type == EdgeType::Acception)
			{
				if (!acception_jume_state)
				{
					acception_jume_state = edge.jump_state;
					return false;
				}
				else {
					acception_jume_state = std::max(*acception_jume_state, edge.acception.jump_state);
					return true;
				}
			}
			else if (acception_jume_state)
			{
				if (edge.type == EdgeType::Comsume && edge.jump_state < *acception_jume_state)
					return true;
			}
			return false;
		}), edges.end());
	}
	
	Table Table::CreateFromRegexs(LexicalRegexInitTuple const* adress, size_t length, bool ignore_controller)
	{
		std::vector<Unfa::Table> unfas;
		unfas.reserve(length);
		for(size_t i = 0; i < length; ++i)
			unfas.push_back(Unfa::CreateUnfaTableFromRegex(adress[i].regex, i, adress[i].mask));
		auto result = Unfa::LinkUnfaTable(unfas.data(), unfas.size());
		if(ignore_controller)
			MakeUpUnfaTable(result);
		return { result.Simplify(LexicalFilter) };
	}

	Table Table::CreateFromRegexsReverse(LexicalRegexInitTuple const* adress, size_t length, bool ignore_controller)
	{
		std::vector<Unfa::Table> unfas;
		unfas.reserve(length);
		for (size_t i = length; i> 0; --i)
			unfas.push_back(Unfa::CreateUnfaTableFromRegex(adress[i - 1].regex, i - 1, adress[i - 1].mask));
		auto result = Unfa::LinkUnfaTable(unfas.data(), unfas.size());
		if(ignore_controller)
			MakeUpUnfaTable(result);
		return { result.Simplify(LexicalFilter) };
	}

	SectionPoint CalculateSectionPoint(std::u32string_view str)
	{
		static Unfa::Table LineTable = Unfa::CreateUnfaTableFromRegex(U"[^\r\n]*?(?:\r\n|\n)").Simplify();
		assert(LineTable);
		SectionPoint point;
		while (!str.empty())
		{
			auto re2 = LineTable.Mark(str);
			point.total_index += re2->capture.string.size();
			if (re2)
			{
				point.line += 1;
				point.line_index = re2->capture.string.size();
				str = { str.begin() + re2->capture.string.size(), str.end() };
			}
			else
			{
				point.line_index += re2->capture.string.size();
				break;
			}
		}
		return point;
	}

	std::optional<March> Table::ProcessOnce(std::u32string_view code) const
	{
		assert(*this);
		auto re = table.Mark(code);
		if(re)
			return March{ re->acception_state, re->acception_mask, re->capture.string, {code.begin() + re->capture.string.size(), code.end()}, {{}, CalculateSectionPoint(re->capture.string)} };
		return std::nullopt;
	}

	std::vector<March> Table::Process(std::u32string_view code, size_t ignore_mask) const
	{
		std::vector<March> result;
		Section current_section;
		while(!code.empty())
		{
			auto re = ProcessOnce(code);
			if(re)
			{
				current_section.start = current_section.end;
				current_section.end = current_section.end + re->section.end;
				if(re->mask != ignore_mask)
				{
					re->section = current_section;
					result.push_back(*re);
				}
				code = re->last_string;
			}else
				throw Error::UnaccaptableLexicalItem{ std::u32string( code.begin(),  code.begin() + std::min(static_cast<size_t>(8), code.size())), current_section };
		}
		return result;
	}
}