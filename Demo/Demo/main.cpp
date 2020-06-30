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

std::u32string_view EbnfCode1();

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

	Ebnf::Table tab2 = Ebnf::CreateTable(EbnfCode1());
	int Testing = 1 + 2 + 3 * 4 - 4 / 2 + 2 * +3 * -2;
	auto His2 = Ebnf::Process(tab2, U"1 + 2 + 3 * 4 - 4 / 2 + 2 * +3 * -2");
	int result2 = std::any_cast<int>(Ebnf::Process(His2, [](Ebnf::Element& e) -> std::any {
		if (e.IsTerminal())
		{
			if (e.shift.mask == 1)
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

	std::cout << result2 << std::endl;

	system("pause");
}

std::u32string_view EbnfCode1()
{
	return UR"(
_IGNORE := '\s'
Num := '(\+|\-)?[1-9][0-9]*' : [1]

%%%

$ := <Exp>

<Exp> := Num : [1]
    := <Exp> '+' <Exp> : [2]
    := <Exp> '*' <Exp> : [3]
    := <Exp> '/' <Exp> : [4]
    := <Exp> '-' <Exp> : [5]
    := '(' <Exp> ')' : [6]

%%%

('*' '/') ('+' '-')
)";
}