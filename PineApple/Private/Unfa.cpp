#include "../Public/Unfa.h"
#include "../Public/Lr0.h"
#include <optional>
#include <variant>
#include <string_view>

namespace PineApple::Unfa
{
	enum class T
	{
		Char = 0,
		Min, // -
		SquareBracketsLeft, //[
		SquareBracketsRight, // ]
		ParenthesesLeft, //(
		ParenthesesRight, //)
		Mulity, //*
		MulityNoGreedy,
		Question, // ?
		QuestionNoGreedy,
		Or, // |
		Add, // +
		AddNoGreedy,
		Not, // ^
		Colon, // :
	};

	constexpr Lr0::Symbol operator*(T sym) { return Lr0::Symbol{ static_cast<size_t>(sym), Lr0::TerminalT{} }; };

	enum class NT
	{
		Statement,
		ExpressionStatement,
		LeftOrStatement,
		RightOrStatement,
		OrStatement,
		CharList,
		CharListNotable,
		Expression,
		NoGreedAppend,
	};

	constexpr Lr0::Symbol operator*(NT sym) { return Lr0::Symbol{ static_cast<size_t>(sym), Lr0::NoTerminalT{} }; };

	const Lr0::Table& rex_lr0()
	{
		static Lr0::Table rex_lr0 = Lr0::CreateTable(
			*NT::Statement,
			{
				{{*NT::Statement, *NT::ExpressionStatement}, 0},
				{{*NT::Statement, *NT::OrStatement}, 0},
				{{*NT::LeftOrStatement, *NT::LeftOrStatement, *NT::Expression}, 1},
				{{*NT::LeftOrStatement, *NT::Expression}, 0},
				{{*NT::RightOrStatement, *NT::Expression, *NT::RightOrStatement}, 1},
				{{*NT::RightOrStatement, *NT::Expression}, 0},
				{{*NT::OrStatement, *NT::LeftOrStatement, *T::Or, *NT::RightOrStatement}, 2},
				{{*NT::OrStatement, *NT::OrStatement, *T::Or, *NT::RightOrStatement}, 2},
				{{*NT::Expression, *T::ParenthesesLeft, *T::Question, *T::Colon, *NT::OrStatement, *T::ParenthesesRight}, 3},
				{{*NT::Expression, *T::ParenthesesLeft, *T::Question, *T::Colon, *NT::ExpressionStatement, *T::ParenthesesRight}, 3},
				{{*NT::Expression, *T::ParenthesesLeft, *T::Question, *T::Colon, *T::ParenthesesRight}, 19},
				{{*NT::Expression, *T::ParenthesesLeft, *T::ParenthesesRight}, 20},
				{{*NT::Expression, *T::ParenthesesLeft, *NT::OrStatement, *T::ParenthesesRight}, 4},
				{{*NT::Expression, *T::ParenthesesLeft, *NT::ExpressionStatement, *T::ParenthesesRight}, 4},
				{{*NT::CharList}, 5},
				{{*NT::CharList, *NT::CharList, *T::Char}, 6},
				{{*NT::CharList, *NT::CharList, *T::Char, *T::Min, *T::Char}, 7},
				{{*NT::Expression, *T::SquareBracketsLeft, *NT::CharList, *T::SquareBracketsRight}, 8},
				{{*NT::Expression, *T::SquareBracketsLeft, *T::Not, *NT::CharList, *T::SquareBracketsRight}, 9},
				{{*NT::Expression, *T::Colon}, 12},
				{{*NT::Expression, *T::Char}, 12},
				{{*NT::Expression, *T::Min}, 12},
				{{*NT::Expression, *T::Not}, 12},
				{{*NT::Expression, *NT::Expression, *T::MulityNoGreedy}, 13},
				{{*NT::Expression, *NT::Expression, *T::AddNoGreedy}, 14},
				{{*NT::Expression, *NT::Expression, *T::QuestionNoGreedy}, 15},
				{{*NT::Expression, *NT::Expression, *T::Mulity}, 16},
				{{*NT::Expression, *NT::Expression, *T::Add}, 17},
				{{*NT::Expression, *NT::Expression, *T::Question}, 18},
				{{*NT::ExpressionStatement, *NT::ExpressionStatement, *NT::Expression}, 1},
				{{*NT::ExpressionStatement, *NT::Expression}, 0},
			}, {}
		);
		return rex_lr0;
	}

	std::tuple<T, Interval> RexLexerTranslater(std::u32string_view::const_iterator& begin, std::u32string_view::const_iterator end)
	{
		assert(begin != end);
		char32_t input = *(begin++);
		switch (input)
		{
		case U'-':return { T::Min, Interval(Segment{input, input + 1}) };
		case U'[': return { T::SquareBracketsLeft, Interval(Segment{input, input + 1}) };
		case U']':return  { T::SquareBracketsRight, Interval(Segment{input, input + 1}) };
		case U'(': return  { T::ParenthesesLeft , Interval(Segment{input, input + 1}) };
		case U')':return { T::ParenthesesRight, Interval(Segment{input, input + 1}) };
		case U'*':
		{
			if(begin != end && *(begin) == U'?')
			{
				++begin;
				return {T::MulityNoGreedy, Interval(Segment{input, input + 1}) };
			}else
				return { T::Mulity, Interval(Segment{input, input + 1}) };
		}
		case U'?':
		{
			if (begin != end && *(begin) == U'?')
			{
				++begin;
				return { T::QuestionNoGreedy, Interval(Segment{input, input + 1}) };
			}
			else
				return { T::Question, Interval(Segment{input, input + 1}) };
		}
		case U'.': return { T::Char, Interval(Segment{1, std::numeric_limits<char32_t>::max()}) };
		case U'|':return { T::Or, Interval(Segment{input, input + 1}) };
		case U'+':
		{
			if (begin != end && *(begin) == U'?')
			{
				++begin;
				return { T::AddNoGreedy, Interval(Segment{input, input + 1}) };
			}
			else
				return { T::Add, Interval(Segment{input, input + 1}) };
		}
		case U'^':return { T::Not, Interval(Segment{input, input + 1}) };
		case U':':return {T::Colon, Interval(Segment{input, input + 1})};
		case U'\\':
		{
			if(begin != end)
			{
				input = *(begin++);
				switch (input)
				{
				case U'd': return { T::Char, Interval(Segment{ U'0', U'9' + 1 }) };
				case U'D': {
					Interval Tem({ {1, U'0'}, {U'9' + 1, std::numeric_limits<char32_t>::max()} });
					return { T::Char, std::move(Tem) };
				};
				case U'f': return { T::Char, Interval(Segment{ U'\f', U'\f' + 1 }) };
				case U'n': return { T::Char, Interval(Segment{ U'\n', U'\n' + 1 }) };
				case U'r': return { T::Char, Interval(Segment{ U'\r', U'\r' + 1 }) };
				case U't': return { T::Char, Interval(Segment{ U'\t', U'\t' + 1 }) };
				case U'v': return { T::Char, Interval(Segment{ U'\v', U'\v' + 1 }) };
				case U's':
				{
					Interval tem({ { 1, 33 }, {127, 128} });
					return { T::Char, std::move(tem) };
				}
				case U'S':
				{
					Interval tem({ {33, 127}, {128, std::numeric_limits<char32_t>::max()} });
					return { T::Char, std::move(tem) };
				}
				case U'w':
				{
					Interval tem({ { U'a', U'z' + 1 }, { U'A', U'Z' + 1 }, { U'_', U'_' + 1} });
					return { T::Char, std::move(tem) };
				}
				case U'W':
				{
					Interval tem({ { U'a', U'z' + 1 }, { U'A', U'Z' + 1 }, { U'_', U'_' + 1} });
					Interval total(Segment{ 1, std::numeric_limits<char32_t>::max() });
					return { T::Char, total - tem };
				}
				case U'u':
				{
					assert(begin + 4 <= end);
					size_t index = 0;
					for (size_t i = 0; i < 4; ++i)
					{
						char32_t in = *(begin++);
						index *= 16;
						if (in >= U'0' && in <= U'9')
							index += in - U'0';
						else if (in >= U'a' && in <= U'f')
							index += in - U'a' + 10;
						else if (in >= U'A' && in <= U'F')
							index += in - U'A' + 10;
						else
							assert(false);
					}
					return { T::Char, Interval(Segment{ static_cast<char32_t>(index), static_cast<char32_t>(index) + 1 }) };
				}
				default:
					Interval tem(Segment{ input,input + 1 });
					return { T::Char, tem };
					break;
				}
			}else
				throw Error::UnaccaptableRexgex{};
			break;
		}
		default:
			return { T::Char, Interval(Segment{input, input + 1}) };
			break;
		}
	}

	std::tuple<std::vector<Lr0::Symbol>, std::vector<Interval>> RexLexer(std::u32string_view Input)
	{
		auto begin = Input.begin();
		auto end = Input.end();
		std::vector<Lr0::Symbol> Symbols;
		std::vector<Interval> RangeSets;
		while (begin != end)
		{
			auto [Sym, Rs] = RexLexerTranslater(begin, end);
			Symbols.push_back(*Sym);
			RangeSets.push_back(Rs);
		}
		return { std::move(Symbols), std::move(RangeSets) };

	}

	Table Table::CreateFromRegex(std::u32string_view rex, size_t acception_state, size_t acception_mask)
	{
		try
		{
			struct TemNode
			{
				size_t in;
				size_t out;
			};
			std::vector<Segment> temporary_interval;
			std::vector<std::vector<Edge>> temporary_node;
			temporary_node.emplace_back();
			auto [symbols, comsumes] = RexLexer(rex);
			auto history = Lr0::Process(rex_lr0(), symbols.data(), symbols.size());
			auto result = Lr0::Process(history, [&](Lr0::Element& tra) -> std::any
			{
				if (tra.IsTerminal())
				{
					return comsumes[tra.shift.token_index];
				}
				else {
					switch (tra.reduce.mask)
					{
					case 0: return tra.MoveRawData(0);
					case 1:
					{
						auto Node1 = tra.GetData<TemNode>(0);
						auto Node2 = tra.GetData<TemNode>(1);
						temporary_node[Node1.out].push_back(Edge{EdgeType::Epsilon, Node2.in});
						return TemNode{Node1.in, Node2.out};
					}
					case 2:
					{
						auto Node1 = tra.GetData<TemNode>(0);
						auto Node2 = tra.GetData<TemNode>(2);
						size_t NewNodeIn = temporary_node.size();
						size_t NewNodeOut = NewNodeIn + 1;
						temporary_node.push_back({Edge{EdgeType::Epsilon, Node1.in}, Edge{EdgeType::Epsilon, Node2.in}});
						temporary_node.push_back({});
						temporary_node[Node1.out].push_back(Edge{ EdgeType::Epsilon, NewNodeOut });
						temporary_node[Node2.out].push_back(Edge{ EdgeType::Epsilon, NewNodeOut });
						return TemNode{NewNodeIn, NewNodeOut};
					}
					case 3:
					{
						return tra.MoveRawData(3);
					}
					case 4:
					{
						auto Node1 = tra.GetData<TemNode>(1);
						size_t NewNodeIn = temporary_node.size();
						size_t NewNodeOut = NewNodeIn + 1;
						temporary_node.push_back({ Edge{EdgeType::Capture, Node1.in, 1, acception_state}});
						temporary_node.push_back({});
						temporary_node[Node1.out].push_back(Edge{ EdgeType::Capture, NewNodeOut, 0, acception_state });
						return TemNode{ NewNodeIn, NewNodeOut };
					}
					case 19:
					{
						size_t NewNodeIn = temporary_node.size();
						size_t NewNodeOut = NewNodeIn + 1;
						temporary_node.push_back({ Edge{EdgeType::Epsilon, NewNodeOut, 0, 0} });
						temporary_node.push_back({});
						return TemNode{ NewNodeIn, NewNodeOut };
					}
					case 20:
					{
						size_t NewNode1 = temporary_node.size();
						size_t NewNode2 = NewNode1 + 1;
						size_t NewNode3 = NewNode2 + 1;
						size_t NewNode4 = NewNode3 + 1;
						temporary_node.push_back({ Edge{EdgeType::Capture, NewNode2, 1, acception_state} });
						temporary_node.push_back({ Edge{EdgeType::Epsilon, NewNode3, 0, 0} });
						temporary_node.push_back({ Edge{EdgeType::Capture, NewNode4, 0, acception_state} });
						temporary_node.push_back({});
						return TemNode{ NewNode1, NewNode4 };
					}
					case 5: {return Interval{}; }
					case 6:
					{
						auto& r1 = tra.GetData<Interval&>(0);
						auto& r2 = tra.GetData<Interval&>(1);
						r1 = r1 | r2;
						return tra.MoveRawData(0);
					}
					case 7:
					{
						auto& r1 = tra.GetData<Interval&>(0);
						auto& r2 = tra.GetData<Interval&>(1);
						auto& r3 = tra.GetData<Interval&>(3);
						r1 = r1 | r2.AsSegment().Expand(r3.AsSegment());
						return tra.MoveRawData(0);
					}
					case 8:
					{
						size_t cur_index = temporary_interval.size();
						auto& r1 = tra.GetData<Interval&>(1);
						temporary_interval.insert(temporary_interval.end(), r1.begin(), r1.end());
						size_t NewNodeIn = temporary_node.size();
						size_t NewNodeOut = NewNodeIn + 1;
						temporary_node.push_back({Edge{EdgeType::Comsume, NewNodeOut, cur_index, r1.size()}});
						temporary_node.emplace_back();
						return TemNode{ NewNodeIn, NewNodeOut };
					}
					case 9:
					{
						size_t cur_index = temporary_interval.size();
						auto& r1 = tra.GetData<Interval&>(2);
						Interval total(Segment{1, std::numeric_limits<char32_t>().max()});
						r1 = total - r1;
						temporary_interval.insert(temporary_interval.end(), r1.begin(), r1.end());
						size_t NewNodeIn = temporary_node.size();
						size_t NewNodeOut = NewNodeIn + 1;
						temporary_node.push_back({Edge{ EdgeType::Comsume, NewNodeOut, cur_index, r1.size() }});
						temporary_node.push_back({});
						return TemNode{ NewNodeIn, NewNodeOut };
					}
					case 12:
					{
						size_t cur_index = temporary_interval.size();
						auto& r1 = tra.GetData<Interval&>(0);
						temporary_interval.insert(temporary_interval.end(), r1.begin(), r1.end());
						size_t NewNodeIn = temporary_node.size();
						size_t NewNodeOut = NewNodeIn + 1;
						temporary_node.push_back({Edge{ EdgeType::Comsume, NewNodeOut, cur_index, r1.size() }});
						temporary_node.push_back({});
						return TemNode{ NewNodeIn, NewNodeOut };
					}
					case 13:
					{
						auto Node = tra.GetData<TemNode>(0);
						size_t NewNodeIn = temporary_node.size();
						size_t NewNodeOut = NewNodeIn + 1;
						temporary_node.emplace_back();
						temporary_node.emplace_back();
						auto& ref = temporary_node[Node.out];
						auto& ref2 = temporary_node[NewNodeIn];
						ref.push_back(Edge{ EdgeType::Epsilon, NewNodeOut });
						ref.push_back(Edge{ EdgeType::Epsilon, Node.in });
						ref2.push_back(Edge{ EdgeType::Epsilon, NewNodeOut });
						ref2.push_back(Edge{ EdgeType::Epsilon, Node.in });
						return TemNode{ NewNodeIn, NewNodeOut };
					}
					case 14:
					{
						auto Node = tra.GetData<TemNode>(0);
						size_t NewNodeIn = temporary_node.size();
						size_t NewNodeOut = NewNodeIn + 1;
						temporary_node.push_back({});
						temporary_node.push_back({});
						auto& ref = temporary_node[Node.out];
						auto& ref2 = temporary_node[NewNodeIn];
						ref.push_back(Edge{ EdgeType::Epsilon, NewNodeOut });
						ref.push_back(Edge{ EdgeType::Epsilon, Node.in });
						ref2.push_back(Edge{ EdgeType::Epsilon, Node.in });
						return TemNode{ NewNodeIn, NewNodeOut };
					}
					case 15:
					{
						auto Node = tra.GetData<TemNode>(0);
						size_t NewNodeIn = temporary_node.size();
						size_t NewNodeOut = NewNodeIn + 1;
						temporary_node.push_back({});
						temporary_node.push_back({});
						auto& ref = temporary_node[Node.out];
						auto& ref2 = temporary_node[NewNodeIn];
						ref.push_back(Edge{ EdgeType::Epsilon, NewNodeOut });
						ref2.push_back(Edge{ EdgeType::Epsilon, NewNodeOut });
						ref2.push_back(Edge{ EdgeType::Epsilon, Node.in });
						return TemNode{ NewNodeIn, NewNodeOut };
					}
					case 16:
					{
						auto Node = tra.GetData<TemNode>(0);
						size_t NewNodeIn = temporary_node.size();
						size_t NewNodeOut = NewNodeIn + 1;
						temporary_node.emplace_back();
						temporary_node.emplace_back();
						auto& ref = temporary_node[Node.out];
						auto& ref2 = temporary_node[NewNodeIn];
						ref.push_back(Edge{ EdgeType::Epsilon, Node.in });
						ref.push_back(Edge{ EdgeType::Epsilon, NewNodeOut });
						ref2.push_back(Edge{ EdgeType::Epsilon, Node.in });
						ref2.push_back(Edge{ EdgeType::Epsilon, NewNodeOut });
						return TemNode{ NewNodeIn, NewNodeOut };
					}
					case 17:
					{
						auto Node = tra.GetData<TemNode>(0);
						size_t NewNodeIn = temporary_node.size();
						size_t NewNodeOut = NewNodeIn + 1;
						temporary_node.push_back({});
						temporary_node.push_back({});
						auto& ref = temporary_node[Node.out];
						auto& ref2 = temporary_node[NewNodeIn];
						ref.push_back(Edge{ EdgeType::Epsilon, Node.in });
						ref.push_back(Edge{ EdgeType::Epsilon, NewNodeOut });
						ref2.push_back(Edge{ EdgeType::Epsilon, Node.in });
						return TemNode{ NewNodeIn, NewNodeOut };
					}
					case 18:
					{
						auto Node = tra.GetData<TemNode>(0);
						size_t NewNodeIn = temporary_node.size();
						size_t NewNodeOut = NewNodeIn + 1;
						temporary_node.push_back({});
						temporary_node.push_back({});
						auto& ref = temporary_node[Node.out];
						auto& ref2 = temporary_node[NewNodeIn];
						ref.push_back(Edge{ EdgeType::Epsilon, NewNodeOut });
						ref2.push_back(Edge{ EdgeType::Epsilon, Node.in });
						ref2.push_back(Edge{ EdgeType::Epsilon, NewNodeOut });
						return TemNode{ NewNodeIn, NewNodeOut };
					}
					default: assert(false); return {};
					}
				}
				return {};
			});
			auto result_node = std::any_cast<TemNode>(result);
			temporary_node[0].push_back({EdgeType::Epsilon, result_node.in});
			auto last_index = temporary_node.size();
			temporary_node.emplace_back();
			temporary_node[result_node.out].emplace_back(Edge{EdgeType::Acception, last_index, acception_state, acception_mask });
			Table table;
			table.character_set = std::move(temporary_interval);
			size_t require_nodes_count = 0;
			for(auto& ite : temporary_node)
				require_nodes_count += ite.size();
			table.edges.reserve(require_nodes_count);
			table.nodes.reserve(temporary_node.size());
			for(auto& ite : temporary_node)
			{
				size_t start = table.edges.size();
				table.edges.insert(table.edges.end(), ite.begin(), ite.end());
				table.nodes.emplace_back(start, ite.size());
			}
			return std::move(table);
		}catch(Error::UnaccaptableRexgex const& US)
		{
			throw Error::UnaccaptableRexgex{ std::u32string(rex), acception_state, acception_mask, rex.size() - 1};
		}
		catch(Lr0::Error::UnaccableSymbol const& Symbol)
		{
			throw Error::UnaccaptableRexgex{std::u32string(rex), acception_state, acception_mask, Symbol.index};
		}
	}

	std::optional<March> Table::Mark(std::u32string_view string, bool greey) const
	{
		assert(*this);
		struct CaptureTuple
		{
			bool is_begin;
			size_t capture_index;
			size_t require_state;
		};
		struct SearchStack
		{
			size_t node;
			size_t edges_index;
			std::u32string_view last_string;
			size_t capture_count;
		};
		std::vector<CaptureTuple> capture_stack;
		std::vector<SearchStack> search_stack;
		search_stack.push_back({ 0,0, string });
		std::optional<March> Acception;
		while (!search_stack.empty())
		{
			SearchStack stack;
			Node node;
			{
				auto cur_ite = search_stack.rbegin();
				stack = *cur_ite;
				node = nodes[cur_ite->node];
				++cur_ite->edges_index;
				assert(cur_ite->edges_index <= node.edge_count);
				if(cur_ite->edges_index == node.edge_count)
					search_stack.pop_back();
			}
			capture_stack.resize(stack.capture_count);
			Edge edge = edges[node.edge_start_index + stack.edges_index];
			switch (edge.type)
			{
			case Table::EdgeType::Comsume:
			{
				if(!stack.last_string.empty())
				{
					auto& ref = edge.comsume;
					IntervalWrapper wrapper(character_set.data() + ref.character_set_start_index, ref.character_set_count);
					auto cur_character = *stack.last_string.begin();
					if(wrapper.IsInclude(cur_character) && nodes[ref.jump_state].edge_count > 0)
						search_stack.push_back({ ref.jump_state, 0, {stack.last_string.data() + 1, stack.last_string.size() - 1}, capture_stack.size() });
				}
			}break;
			case Table::EdgeType::Acception: {
				auto& ref = edge.acception;
				search_stack.clear();
				std::u32string_view capture_str( string.data(), string.size() - stack.last_string.size());
				if(greey)
				{
					++stack.edges_index;
					if (stack.edges_index  < node.edge_count)
						search_stack.push_back(stack);
					if(nodes[ref.jump_state].edge_count > 0)
						search_stack.push_back({ ref.jump_state, 0, stack.last_string, capture_stack.size() });
					if (!Acception || Acception->capture.string.size() < capture_str.size())
					{
						Acception = March{
							{capture_str, 0},
							ref.acception_state,
							ref.acception_mask,
							{}
						};
					}
				}else
				{
					Acception = March{
							{capture_str, 0},
							ref.acception_state,
							ref.acception_mask,
							{}
					};
					break;
				}
			} break;
			case EdgeType::Capture:
			{
				auto ref = edge.capture;
				capture_stack.push_back({ ref.is_begin == 1, string.size() - stack.last_string.size(), ref.require_state });
			}
			case EdgeType::Epsilon:
			{
				auto ref = edge.epsilon;
				if (nodes[ref.jump_state].edge_count > 0)
					search_stack.push_back({ edge.jump_state, 0, stack.last_string, capture_stack.size() });
			}break;
			default: {assert(false); } break;
			}
		}
		if(Acception)
		{
			std::vector<CaptureTuple> handle_stack;
			for(auto& ite : capture_stack)
			{
				if(ite.require_state == Acception->acception_state)
				{
					if (!ite.is_begin)
					{
						assert(!handle_stack.empty() && handle_stack.rbegin()->is_begin);
						auto start_index = handle_stack.rbegin()->capture_index;
						Acception->sub_capture.emplace_back(March::Sub{ std::u32string_view{string.data() + start_index, string.data() + ite.capture_index}, start_index });
						handle_stack.pop_back();
					}
					else
						handle_stack.push_back(ite);
				}
			}
		}
		return Acception;
	}

	std::tuple<std::set<size_t>, std::vector<Table::Edge>> Table::SearchThroughEpsilonEdge(size_t const* require_state, size_t length) const
	{
		std::vector<Edge> all_edge;
		struct SearchTuple
		{
			size_t node;
			size_t edge;
		};
		std::set<size_t> Finded;
		std::vector<SearchTuple> search_stack;
		
		for(size_t i = length; i > 0; --i)
			search_stack.push_back({ require_state[i-1], 0});
		while(!search_stack.empty())
		{
			auto& ref = *search_stack.rbegin();
			assert(ref.node < nodes.size());
			auto node = nodes[ref.node];
			if(ref.edge >= node.edge_count)
			{
				Finded.insert(ref.node);
				search_stack.pop_back();
			}else while(ref.edge < node.edge_count)
			{
				auto edge = edges[node.edge_start_index + (ref.edge++)];
				if(edge.type == EdgeType::Epsilon)
				{
					if(Finded.find(edge.jump_state) == Finded.end())
						search_stack.push_back({ edge.jump_state, 0 });
					break;
				}else
					all_edge.push_back(edge);
			}
		}
		return { std::move(Finded), std::move(all_edge)};
	}

	std::vector<std::tuple<Interval, std::vector<size_t>>> Table::MergeComsumeEdge(Edge const* tar_edges, size_t edges_length) const
	{
		std::vector<std::tuple<Interval, std::vector<size_t>>> output_edge;
		for(size_t i = 0; i < edges_length; ++i)
		{
			auto cur_edge = tar_edges[i].comsume;
			assert(cur_edge.type == EdgeType::Comsume);
			std::vector<std::tuple<Interval, std::vector<size_t>>> temporary = std::move(output_edge);
			Interval cur(character_set.data() + cur_edge.character_set_start_index, cur_edge.character_set_count, NoDetectT{});
			for(auto& ite : temporary)
			{
				if(!cur)
					output_edge.push_back(std::move(ite));
				auto& [interval, list] = ite;
				auto result = cur.Collision(interval);
				cur = std::move(result.left);
				if(result.middle)
				{
					auto new_list = list;
					assert(std::find(new_list.begin(), new_list.end(), cur_edge.jump_state) == new_list.end());
					new_list.push_back(cur_edge.jump_state);
					output_edge.push_back({std::move(result.middle), std::move(new_list)});
				}
				if(result.right)
					output_edge.push_back({ std::move(result.right), std::move(list) });
			}
			if(cur)
				output_edge.push_back({std::move(cur), {cur_edge.jump_state}} );
		}
		return {std::move(output_edge) };
	}

	Table Table::Link(Table const* other_table, size_t table_size)
	{
		size_t character_set_size = 0;
		size_t edges_size = 0;
		size_t nodes_size = 0;
		size_t total_append_table = 0;
		for(size_t i = 0; i < table_size; ++i)
		{
			auto& ref = other_table[i];
			if(ref)
			{
				character_set_size += ref.character_set.size();
				edges_size += ref.edges.size();
				nodes_size += ref.nodes.size();
				total_append_table += 1;
			}
		}
		
		if(total_append_table > 0)
		{
			Table new_table;
			new_table.character_set.reserve(character_set_size);
			new_table.edges.reserve(total_append_table + edges_size);
			new_table.edges.resize(total_append_table, Edge{EdgeType::Epsilon, 0, 0, 0});
			new_table.nodes.reserve(nodes_size + 1);
			new_table.nodes.push_back({ 0, total_append_table });

			{
				size_t nodes_added = 0;
				for (size_t i = 0; i < table_size; ++i)
				{
					auto& ref = other_table[i];
					if (ref)
					{
						size_t current_nodes = new_table.nodes.size();
						size_t current_edges = new_table.edges.size();
						size_t current_character_set = new_table.character_set.size();
						new_table.nodes.insert(new_table.nodes.end(), ref.nodes.begin(), ref.nodes.end());
						new_table.edges.insert(new_table.edges.end(), ref.edges.begin(), ref.edges.end());
						new_table.character_set.insert(new_table.character_set.end(), ref.character_set.begin(), ref.character_set.end());
						for(size_t i = current_nodes; i < new_table.nodes.size(); ++i)
							new_table.nodes[i].edge_start_index += current_edges;
						for (size_t i = current_edges; i < new_table.edges.size(); ++i)
						{
							auto& edge = new_table.edges[i];
							edge.jump_state += current_nodes;
							if(edge.type == EdgeType::Comsume)
								edge.comsume.character_set_start_index += current_character_set;
						}
						new_table.edges[nodes_added++].jump_state = current_nodes;
					}
				}
			}
			return std::move(new_table);
		}else
		{
			return {};
		}
	}

	void Table::DefaultFilter(Table const&, std::vector<Edge>& edges)
	{
		bool finded_acception = false;
		edges.erase(std::remove_if(edges.begin(), edges.end(), [&](Edge edge)
		{
			if (edge.type == EdgeType::Acception)
			{
				auto old_finded_acception = finded_acception;
				finded_acception = true;
				return old_finded_acception;
			}
			return false;
		}), edges.end());
	}

	Table Table::Simplify(std::function<void(Table const&, std::vector<Edge>&)> edge_filter) const
	{
		assert(*this);
		struct RedefineStateInfo
		{
			size_t new_state;
			std::vector<Edge> Edges;
		};

		std::map<std::set<size_t>, size_t> redefine_state;
		std::vector<std::tuple<size_t, std::vector<Edge>>> search;
		std::vector<Segment> segment_set;
		std::vector<Segment> temporary_segment_list;
		std::vector<std::vector<Edge>> temporary_node;

		auto InsertNewStateFuncion = [&](std::set<size_t> set, std::vector<Edge> edges) -> size_t
		{
			auto re = redefine_state.insert({std::move(set), redefine_state.size()});
			if(re.second)
			{
				temporary_node.emplace_back();
				search.push_back({ re.first->second, std::move(edges)});
			}
			return re.first->second;
		};
		
		{
			size_t start_index = 0;
			auto [sets, edges] = SearchThroughEpsilonEdge(&start_index, 1);
			InsertNewStateFuncion(std::move(sets), std::move(edges));
		}
		
		while(!search.empty())
		{
			auto [state_set, edges] = std::move(*search.rbegin());
			search.pop_back();
			if(edge_filter)
				edge_filter(*this, edges);
			auto ite = edges.begin();
			std::optional<size_t> AcceptionState;
			while(ite != edges.end())
			{
				auto cur = ite;
				while(cur != edges.end() && cur->type == EdgeType::Comsume)
					++cur;
				if(cur != ite)
				{
					auto MergeResult = MergeComsumeEdge(&(*ite), cur - ite);
					for(auto& ite2 : MergeResult)
					{
						auto& [inv, temporary_list] = ite2;
						//std::vector<size_t> temporary_list(old_state_set.begin(), old_state_set.end());
						auto [search_state, search_edge] = SearchThroughEpsilonEdge(temporary_list.data(), temporary_list.size());
						size_t state = InsertNewStateFuncion(std::move(search_state), std::move(search_edge));
						size_t cur_list = temporary_segment_list.size();
						temporary_segment_list.insert(temporary_segment_list.end(), inv.begin(), inv.end());
						temporary_node[state_set].push_back({EdgeType::Comsume, state, cur_list, inv.size()});
						// todo list
					}
				}
				if(cur != edges.end())
				{
					auto [search_state, search_edge] = SearchThroughEpsilonEdge(&cur->jump_state, 1);
					size_t state = InsertNewStateFuncion(std::move(search_state), std::move(search_edge));
					temporary_node[state_set].push_back({cur->type, state, cur->s1, cur->s2});
					++cur;
				}
				ite = cur;
			}
		}

		Table table;
		table.character_set = std::move(temporary_segment_list);
		table.nodes.reserve(temporary_node.size());
		for(auto& ite : temporary_node)
		{
			size_t cur = table.edges.size();
			table.edges.insert(table.edges.end(), ite.begin(), ite.end());
			table.nodes.push_back({cur, ite.size()});
		}
		return std::move(table);
	}

	DebuggerTable::DebuggerTable(Table const& table)
	{
		nodes.resize(table.nodes.size());
		for(size_t i = 0; i < table.nodes.size(); ++i)
		{
			auto& self = nodes[i];
			auto& input = table.nodes[i];
			self.edges.reserve(input.edge_count);
			auto edge_start = table.edges.data() + input.edge_start_index;
			for(size_t i =0; i < input.edge_count; ++i)
			{
				if(edge_start[i].type == Table::EdgeType::Comsume)
					self.edges.push_back({ edge_start[i], IntervalWrapper{table.character_set.data() + edge_start[i].comsume.character_set_start_index, edge_start[i].comsume.character_set_count}});
				else
					self.edges.push_back({ edge_start[i], {}});
			}
		}
	}
}
