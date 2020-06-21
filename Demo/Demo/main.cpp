#include "../../PineApple/Interface/Lr0.h"
#include "../../PineApple/Interface/CharEncode.h"
#include "../../PineApple/Interface/Nfa.h"
#include "../../PineApple/Interface/Ebnf.h"
#include <iostream>
#include <fstream>
#include <filesystem>

using namespace PineApple;


enum class Noterminal
{
	Exp = 0,
};

constexpr Lr0::Symbol operator*(Noterminal input) { return Lr0::Symbol(static_cast<size_t>(input), Lr0::noterminal); }

enum class Terminal
{
	Num = 0,
	Add,
	Multi
};

constexpr Lr0::Symbol operator*(Terminal input) { return Lr0::Symbol(static_cast<size_t>(input), Lr0::terminal); }

int StrToInt(std::u32string_view input)
{
	auto str = CharEncode::Wrapper(input).To<char>();
	int i = 0;
	sscanf_s(str.c_str(), "%i", &i);
	return i;
}

int main()
{

	std::u32string_view rexs[] = {
		U"(\\+|\\-)?[1-9][0-9]*",
		U"\\+",
		U"\\*",
		U"\\s",
	};

	Nfa::Table n_tab = Nfa::CreateTableFromRex(rexs, 4);

	auto StrElement = Nfa::Process(n_tab, U"1 + +10 * -2");

	std::vector<Lr0::Symbol> Syms;
	std::vector<int> Datas;

	for (auto& Ite : StrElement)
	{
		if (Ite.acception != 3)
		{
			Syms.push_back(Lr0::Symbol(Ite.acception, Lr0::TerminalT{}));
			if (Ite.acception == 0)
				Datas.push_back(StrToInt(Ite.capture));
			else
				Datas.push_back(0);
		}
	}

	Lr0::Table tab = Lr0::CreateTable(
		*Noterminal::Exp,
	{
		{{*Noterminal::Exp, *Terminal::Num}, 1},
		{{*Noterminal::Exp, *Noterminal::Exp, *Terminal::Add, *Noterminal::Exp}, 2},
		{{*Noterminal::Exp, *Noterminal::Exp, *Terminal::Multi, *Noterminal::Exp}, 3},
	},
		{ {{*Terminal::Multi}}, {{*Terminal::Add}} }
	);

	auto His = Lr0::Process(tab, Syms.data(), Syms.size());

	int result = std::any_cast<int>(Lr0::Process(His, [&](Lr0::Element& E) -> std::any {
		if (E.IsTerminal())
		{
			if (E.value == *Terminal::Num)
				return Datas[E.shift.token_index];
		}
		else {
			switch (E.reduce.mask)
			{
			case 1: return std::move(E.GetRawData(0));
			case 2: return E.GetData<int>(0) + E.GetData<int>(2);
			case 3: return E.GetData<int>(0) * E.GetData<int>(2);
			default:
				break;
			}
		}
		return {};
	}));

	std::cout << result << std::endl;

	std::optional<std::filesystem::path> EbnfFile;

	{
		auto fs = std::filesystem::current_path();
		fs = fs.parent_path();
		fs = fs.parent_path();
		fs = fs.parent_path();

		for (auto Path : std::filesystem::recursive_directory_iterator(fs))
		{
			if (Path.is_regular_file())
			{
				auto Name = Path.path().filename();
				if (Name.generic_u32string() == U"expression.ebnf")
					EbnfFile = Path.path();
			}
		}
	}

	if (EbnfFile)
	{
		std::ifstream file(*EbnfFile, std::ios::binary);
		assert(file.is_open());
		auto fs = std::filesystem::file_size(*EbnfFile);
		std::vector<std::byte> Datas;
		Datas.resize(fs);
		file.read(reinterpret_cast<char*>(Datas.data()), Datas.size());
		file.close();
		auto [type, binary, size] = CharEncode::FixBinaryWithBom(Datas.data(), Datas.size());
		assert(type == CharEncode::BomType::UTF8);
		auto Re = CharEncode::Wrapper(reinterpret_cast<char const*>(binary), size).To<char32_t>();
		Ebnf::Table tab = Ebnf::CreateTable(Re);
		int Testing = 1 + 2 + 3 * 4 - 4 / 2 + 2 * +3 * -2;
		auto His = Ebnf::Process(tab, U"1 + 2 + 3 * 4 - 4 / 2 + 2 * +3 * -2");
		int result = std::any_cast<int>(Ebnf::Process(His, [](Ebnf::Element& e) -> std::any {
			if (e.IsTerminal())
			{
				if (e.string == U"Num")
					return StrToInt(e.shift.capture);
			}
			else {
				switch (e.reduce.mask)
				{
				case 1: return std::move(e.GetRawData(0));
				case 2: return e.GetData<int>(0) + e.GetData<int>(2);
				case 3: return e.GetData<int>(0) * e.GetData<int>(2);
				case 4: return e.GetData<int>(0) / e.GetData<int>(2);
				case 5: return e.GetData<int>(0) - e.GetData<int>(2);
				case 6: return std::move(e.GetRawData(1));
				}
			}
			return {};
		}));

		std::cout << result << std::endl;
	}

	system("pause");
}