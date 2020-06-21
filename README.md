# PineApple
字符，文本处理库

Demo 地址见 `Demo/Demo.sln(vs2019)`

包含以下几个功能库：

* CharEncode 字符编码

    支持UTF8，UTF16，UTF32之间的相互转换，在Win下支持ANSI到WideCharacter之间的相互转换。

    具体用法，如UTF32到UTF8：
    ```cpp
    using namespace PineApple;

    char32_t const* Str;
    size_t StrLength;
    std::string Result = CharEncode::Wrapper<char32_t>(Str, StrLength).To<char>()
    ```
    在后续版本会将char转成char8_t

* StrFormat fmt类似物

    对UTF32特化，在CPP20并且编译器支持前的替代版本，只支持简单的语法。

    具体用法：
     ```cpp
    using namespace PineApple;
    namespace StrFormat
    {
        template<>
        struct StrFormat::Formatter<YourType>
        {
            std::u32string operator()(std::u32string_view paras, YourType const& )
            {
                return U"WTF";
            }
        }
    }
     
    auto Pattern = StrFormat::CreatePatternRef(U"abc{-hex}, {}");
    std::u32string Result = StrFormat::Process(Pattern, 1, YourType{});
    // abc1, WTF
    ```

* Nfa 词法分析器
    
    特化char32_t支持，基于基础正则表达式的词法分析器。

    使用方法：
    ```cpp
    using namespace PineApple;
    std::u32string_view rexs[] = {
		U"(\\+|\\-)?[1-9][0-9]*",
		U"\\+",
		U"\\*",
		U"\\s",
	};

	Nfa::Table n_tab = CreateTableFromRex(rexs, 4);

	auto StrElement = Nfa::Process(n_tab, U"1 + +10 * -2");
    ```

* Lr0 语法分析器

    基于Lr0，带回溯，支持运算符优先级，支持一些二义性文法。

    使用方法：

    ```cpp
    using namespace PineApple;
    int StrToInt(std::u32string_view input)
    {
        auto str = CharEncode::Wrapper(input).To<char>();
        int i = 0;
        sscanf_s(str.c_str(), "%i", &i);
        return i;
    }

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
    ```

* Ebnf Ebnf分析器

    基于UTF32，根据Ebnf文法，生成对应的此法分析器和语法分析器。

    使用方法：

    ```cpp
    std::u32srinrg_view code = 
    R"(
        _IGNORE := '\s'
        Num := '(\+|\-)?[1-9][0-9]*'

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

    Ebnf::Table tab = Ebnf::CreateTable(Re);
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
    ```
