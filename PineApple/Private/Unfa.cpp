#include "../Public/Unfa.h"
#include "../Public/Lr0.h"
#include <optional>
#include <variant>
#include <string_view>
#include <deque>

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

	Table Table::CreateFromRegex(std::u32string_view rex, uint32_t acception_index, uint32_t acception_mask)
	{
		try
		{
			struct TemNode
			{
				uint32_t in;
				uint32_t out;
			};
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
						temporary_node[Node1.out].emplace_back(Edge{Node2.in,  EEpsilon{}});
						return TemNode{Node1.in, Node2.out};
					}
					case 2:
					{
						auto Node1 = tra.GetData<TemNode>(0);
						auto Node2 = tra.GetData<TemNode>(2);
						uint32_t NewNodeIn = static_cast<uint32_t>(temporary_node.size());
						uint32_t NewNodeOut = NewNodeIn + 1;
						temporary_node.push_back({Edge{Node1.in, EEpsilon{}}, Edge{Node2.in, EEpsilon{}}});
						temporary_node.emplace_back();
						temporary_node[Node1.out].emplace_back(Edge{ NewNodeOut, EEpsilon{}});
						temporary_node[Node2.out].emplace_back(Edge{ NewNodeOut, EEpsilon{}});
						return TemNode{NewNodeIn, NewNodeOut};
					}
					case 3:
					{
						return tra.MoveRawData(3);
					}
					case 4:
					{
						auto Node1 = tra.GetData<TemNode>(1);
						uint32_t NewNodeIn = static_cast<uint32_t>(temporary_node.size());
						uint32_t NewNodeOut = NewNodeIn + 1;
						temporary_node.push_back({Edge{Node1.in, ECapture{true, acception_index }}});
						temporary_node.emplace_back();
						temporary_node[Node1.out].emplace_back(Edge{NewNodeOut, ECapture{0, acception_index }});
						return TemNode{ NewNodeIn, NewNodeOut };
					}
					case 19:
					{
						uint32_t NewNodeIn = static_cast<uint32_t>(temporary_node.size());
						uint32_t NewNodeOut = NewNodeIn + 1;
						temporary_node.push_back({Edge{NewNodeOut, EEpsilon{}}});
						temporary_node.emplace_back();
						return TemNode{ NewNodeIn, NewNodeOut };
					}
					case 20:
					{
						uint32_t NewNode1 = static_cast<uint32_t>(temporary_node.size());
						uint32_t NewNode2 = NewNode1 + 1;
						uint32_t NewNode3 = NewNode2 + 1;
						uint32_t NewNode4 = NewNode3 + 1;
						temporary_node.push_back({Edge{NewNode2, ECapture{true, acception_index}}});
						temporary_node.push_back({Edge{NewNode3, EEpsilon{}}});
						temporary_node.push_back({Edge{NewNode4, ECapture{false, acception_index}}});
						temporary_node.emplace_back();
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
						uint32_t NewNodeIn = static_cast<uint32_t>(temporary_node.size());
						uint32_t NewNodeOut = NewNodeIn + 1;
						temporary_node.push_back({ Edge{ NewNodeOut, ECapture{tra.MoveData<Interval>(1)} } });
						temporary_node.emplace_back();
						return TemNode{ NewNodeIn, NewNodeOut };
					}
					case 9:
					{
						Interval r1 = tra.MoveData<Interval>(2);
						Interval total(Segment{1, std::numeric_limits<char32_t>().max()});
						r1 = total - r1;
						uint32_t NewNodeIn = static_cast<uint32_t>(temporary_node.size());
						uint32_t NewNodeOut = NewNodeIn + 1;
						temporary_node.push_back({ Edge{ NewNodeOut, EComsume{std::move(r1)} } });
						temporary_node.emplace_back();
						return TemNode{ NewNodeIn, NewNodeOut };
					}
					case 12:
					{
						uint32_t NewNodeIn = static_cast<uint32_t>(temporary_node.size());
						uint32_t NewNodeOut = NewNodeIn + 1;
						temporary_node.push_back({ Edge{ NewNodeOut, EComsume{std::move(tra.MoveData<Interval>(0))}} });
						temporary_node.emplace_back();
						return TemNode{ NewNodeIn, NewNodeOut };
					}
					case 13:
					{
						auto Node = tra.GetData<TemNode>(0);
						uint32_t NewNodeIn = static_cast<uint32_t>(temporary_node.size());
						uint32_t NewNodeOut = NewNodeIn + 1;
						temporary_node.emplace_back();
						temporary_node.emplace_back();
						auto& ref = temporary_node[Node.out];
						auto& ref2 = temporary_node[NewNodeIn];
						ref.emplace_back(Edge{ NewNodeOut, EEpsilon{} });
						ref.emplace_back(Edge{ Node.in, EEpsilon{} });
						ref2.emplace_back(Edge{ NewNodeOut, EEpsilon{} });
						ref2.emplace_back(Edge{ Node.in, EEpsilon{} });
						return TemNode{ NewNodeIn, NewNodeOut };
					}
					case 14:
					{
						auto Node = tra.GetData<TemNode>(0);
						uint32_t NewNodeIn = static_cast<uint32_t>(temporary_node.size());
						uint32_t NewNodeOut = NewNodeIn + 1;
						temporary_node.emplace_back();
						temporary_node.emplace_back();
						auto& ref = temporary_node[Node.out];
						auto& ref2 = temporary_node[NewNodeIn];
						ref.emplace_back(Edge{ NewNodeOut, EEpsilon{} });
						ref.emplace_back(Edge{ Node.in, EEpsilon{} });
						ref2.emplace_back(Edge{ Node.in, EEpsilon{} });
						return TemNode{ NewNodeIn, NewNodeOut };
					}
					case 15:
					{
						auto Node = tra.GetData<TemNode>(0);
						uint32_t NewNodeIn = static_cast<uint32_t>(temporary_node.size());
						uint32_t NewNodeOut = NewNodeIn + 1;
						temporary_node.emplace_back();
						temporary_node.emplace_back();
						auto& ref = temporary_node[Node.out];
						auto& ref2 = temporary_node[NewNodeIn];
						ref.emplace_back(Edge{ NewNodeOut, EEpsilon{} });
						ref2.emplace_back(Edge{ NewNodeOut, EEpsilon{} });
						ref2.emplace_back(Edge{ Node.in, EEpsilon{} });
						return TemNode{ NewNodeIn, NewNodeOut };
					}
					case 16:
					{
						auto Node = tra.GetData<TemNode>(0);
						uint32_t NewNodeIn = static_cast<uint32_t>(temporary_node.size());
						uint32_t NewNodeOut = NewNodeIn + 1;
						temporary_node.emplace_back();
						temporary_node.emplace_back();
						auto& ref = temporary_node[Node.out];
						auto& ref2 = temporary_node[NewNodeIn];
						ref.emplace_back(Edge{ Node.in, EEpsilon{} });
						ref.emplace_back(Edge{ NewNodeOut, EEpsilon{} });
						ref2.emplace_back(Edge{ Node.in, EEpsilon{} });
						ref2.emplace_back(Edge{ NewNodeOut, EEpsilon{} });
						return TemNode{ NewNodeIn, NewNodeOut };
					}
					case 17:
					{
						auto Node = tra.GetData<TemNode>(0);
						uint32_t NewNodeIn = static_cast<uint32_t>(temporary_node.size());
						uint32_t NewNodeOut = NewNodeIn + 1;
						temporary_node.emplace_back();
						temporary_node.emplace_back();
						auto& ref = temporary_node[Node.out];
						auto& ref2 = temporary_node[NewNodeIn];
						ref.emplace_back(Edge{ Node.in, EEpsilon{} });
						ref.emplace_back(Edge{ NewNodeOut, EEpsilon{} });
						ref2.emplace_back(Edge{ Node.in, EEpsilon{} });
						return TemNode{ NewNodeIn, NewNodeOut };
					}
					case 18:
					{
						auto Node = tra.GetData<TemNode>(0);
						uint32_t NewNodeIn = static_cast<uint32_t>(temporary_node.size());
						uint32_t NewNodeOut = NewNodeIn + 1;
						temporary_node.emplace_back();
						temporary_node.emplace_back();
						auto& ref = temporary_node[Node.out];
						auto& ref2 = temporary_node[NewNodeIn];
						ref.emplace_back(Edge{ NewNodeOut, EEpsilon{} });
						ref2.emplace_back(Edge{ Node.in, EEpsilon{} });
						ref2.emplace_back(Edge{  NewNodeOut, EEpsilon{} });
						return TemNode{ NewNodeIn, NewNodeOut };
					}
					default: assert(false); return {};
					}
				}
				return {};
			});
			auto result_node = std::any_cast<TemNode>(result);
			temporary_node[0].emplace_back(Edge{result_node.in, EEpsilon{}});
			auto last_index = static_cast<uint32_t>(temporary_node.size());
			temporary_node.emplace_back();
			temporary_node[result_node.out].emplace_back(Edge{ last_index, EAcception{acception_index, acception_mask} });
			return {std::move(temporary_node)};
		}
		catch(Lr0::Error::UnaccableSymbol const& Symbol)
		{
			throw Error::UnaccaptableRexgex{std::u32string(rex), acception_index, acception_mask, Symbol.index};
		}
	}

	/*
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
		std::deque<CaptureTuple> capture_stack;
		std::deque<SearchStack> search_stack;
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
	*/

	std::tuple<std::set<uint32_t>, std::vector<Table::Edge>> Table::SearchThroughEpsilonEdge(uint32_t const* require_state, size_t length) const
	{
		std::vector<Edge> all_edge;
		struct SearchTuple
		{
			uint32_t node;
			uint32_t edge;
		};
		std::set<uint32_t> finded;
		std::vector<SearchTuple> search_stack;

		for (uint32_t i = static_cast<uint32_t>(length); i > 0; --i)
			search_stack.push_back({ require_state[i - 1], 0 });
		while (!search_stack.empty())
		{
			auto& ref = *search_stack.rbegin();
			assert(ref.node < nodes.size());
			auto& node = nodes[ref.node];
			if (ref.edge >= node.size())
			{
				finded.insert(ref.node);
				search_stack.pop_back();
			}
			else while (ref.edge < node.size())
			{
				auto edge = node[(ref.edge++)];
				if (edge.Is<EEpsilon>())
				{
					if (finded.find(edge.jump_state) == finded.end())
						search_stack.push_back({ edge.jump_state, 0 });
					break;
				}
				else
					all_edge.push_back(edge);
			}
		}
		return { std::move(finded), std::move(all_edge) };
	}

	std::vector<std::tuple<Interval, std::vector<uint32_t>>> Table::MergeComsumeEdge(Edge const* tar_edges, size_t edges_length) const
	{
		std::vector<std::tuple<Interval, std::vector<uint32_t>>> output_edge;
		for(size_t i = 0; i < edges_length; ++i)
		{
			auto& cur_edge = tar_edges[i];
			assert(cur_edge.Is<EComsume>());
			std::vector<std::tuple<Interval, std::vector<uint32_t>>> temporary = std::move(output_edge);
			Interval cur(cur_edge.Get<EComsume>().interval);
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
		size_t edge_count = 1;
		for(size_t i = 0; i < table_size; ++i)
		{
			if(other_table[i])
				edge_count += other_table[i].nodes.size();
		}
		if(edge_count > 3)
		{
			std::vector<std::vector<Edge>> new_node;
			new_node.reserve(edge_count);
			new_node.emplace_back();
			for (size_t i = 0; i < table_size; ++i)
			{
				auto& ref = other_table[i];
				if (ref)
				{
					uint32_t cur_node = static_cast<uint32_t>(new_node.size());
					new_node.insert(new_node.end(), ref.nodes.begin(), ref.nodes.end());
					for (size_t k = cur_node; k < new_node.size(); ++k)
					{
						for (auto& ite : new_node[k])
							ite.jump_state += cur_node;
					}
					new_node[0].emplace_back(Edge{ cur_node, EEpsilon{} });
				}
			}
			return {std::move(new_node)};
		}
		return {};
	}

	void Table::DefaultFilter(Table const&, std::vector<Edge>& edges)
	{
		bool finded_acception = false;
		edges.erase(std::remove_if(edges.begin(), edges.end(), [&](Edge edge)
		{
			if (edge.Is<EAcception>())
			{
				if(!finded_acception)
				{
					finded_acception = true;
					return false;
				}else
					return true;
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

		std::map<std::set<uint32_t>, uint32_t> redefine_state;
		std::deque<std::tuple<uint32_t, std::vector<Edge>>> search;
		std::vector<Segment> segment_set;
		std::vector<std::vector<Edge>> temporary_node;

		auto InsertNewStateFuncion = [&](std::set<uint32_t> set, std::vector<Edge> edges) -> uint32_t
		{
			auto re = redefine_state.insert({std::move(set), static_cast<uint32_t>(redefine_state.size())});
			if(re.second)
			{
				temporary_node.emplace_back();
				search.push_back({ re.first->second, std::move(edges)});
			}
			return re.first->second;
		};
		
		{
			uint32_t start_index = 0;
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
			std::optional<uint32_t> AcceptionState;
			while(ite != edges.end())
			{
				auto cur = ite;
				while(cur != edges.end() && cur->Is<EComsume>())
					++cur;
				if(cur != ite)
				{
					auto MergeResult = MergeComsumeEdge(&(*ite), cur - ite);
					for(auto& ite2 : MergeResult)
					{
						auto& [inv, temporary_list] = ite2;
						//std::vector<size_t> temporary_list(old_state_set.begin(), old_state_set.end());
						auto [search_state, search_edge] = SearchThroughEpsilonEdge(temporary_list.data(), temporary_list.size());
						uint32_t state = InsertNewStateFuncion(std::move(search_state), std::move(search_edge));
						temporary_node[state_set].emplace_back(Edge{state, EComsume{std::move(inv)}});
						// todo list
					}
				}
				if(cur != edges.end())
				{
					auto [search_state, search_edge] = SearchThroughEpsilonEdge(&(cur->jump_state), 1);
					uint32_t state = InsertNewStateFuncion(std::move(search_state), std::move(search_edge));
					temporary_node[state_set].emplace_back(Edge{state, cur->property});
					++cur;
				}
				ite = cur;
			}
		}
		return { std::move(temporary_node) };
	}
	
	struct SEAcception{ uint32_t acception_index; uint32_t acception_mask; };
	struct SECapture { uint32_t begin; uint32_t require_index; };
	struct SEComsume { uint32_t count; };

	struct SENode{ uint32_t edge_start_offset; uint32_t edge_count; };

	SerilizedTable::SerilizedTable(Table const& table)
	{
		if(table)
		{
			size_t total_space = 0;

			total_space += table.nodes.size() * sizeof(SENode);
			for (auto& ite : table.nodes)
			{
				for (auto& ite2 : ite)
				{
					total_space += sizeof(SEEdgeDescription);
					if (ite2.Is<Table::EAcception>())
						total_space += sizeof(SEAcception);
					else if (ite2.Is<Table::ECapture>())
						total_space += sizeof(SECapture);
					else if (ite2.Is<Table::EComsume>())
					{
						total_space += sizeof(SEComsume);
						auto& ref = ite2.Get<Table::EComsume>();
						total_space += ref.interval.size() * sizeof(Segment);
					}
				}
			}

			node_count = table.nodes.size();
			datas.resize(total_space);
			SENode *node_adress = reinterpret_cast<SENode*>(datas.data());
			std::byte *edges_adress = reinterpret_cast<std::byte*>(node_adress + node_count);
			for(auto& ite : table.nodes)
			{
				*(node_adress++) = {
					static_cast<uint32_t>((reinterpret_cast<size_t>(edges_adress) - reinterpret_cast<size_t>(datas.data())) / sizeof(uint32_t)),
					static_cast<uint32_t>(ite.size())
				};
				for(auto& ite2 : ite)
				{
					auto& edge_desc = *reinterpret_cast<SEEdgeDescription*>(edges_adress);
					edges_adress += sizeof(SEEdgeDescription);
					edge_desc.jump_state = ite2.jump_state;
					if(ite2.Is<Table::EAcception>())
					{
						edge_desc.type = SEdgeType::Acception;
						auto& acception = *reinterpret_cast<SEAcception*>(edges_adress);
						edges_adress += sizeof(SEAcception);
						auto& ref = ite2.Get<Table::EAcception>();
						acception.acception_index = ref.acception_index;
						acception.acception_mask = ref.acception_mask;
					}else if(ite2.Is<Table::ECapture>())
					{
						edge_desc.type = SEdgeType::Capture;
						auto& capture = *reinterpret_cast<SECapture*>(edges_adress);
						edges_adress += sizeof(SECapture);
						auto& ref = ite2.Get<Table::ECapture>();
						capture.begin = (ref.begin ? 1 : 0);
						capture.require_index = ref.require_index;
					}else if(ite2.Is<Table::EComsume>())
					{
						edge_desc.type = SEdgeType::Comsume;
						auto& comsume = *reinterpret_cast<SEComsume*>(edges_adress);
						edges_adress += sizeof(SEComsume);
						auto& ref = ite2.Get<Table::EComsume>();
						comsume.count = static_cast<uint32_t>(ref.interval.size());
						std::memcpy(edges_adress, ref.interval.AsWrapper().begin(), sizeof(Segment) * comsume.count);
						edges_adress += sizeof(Segment) * comsume.count;
					}
				}
			}
		}
	}

	std::optional<March> SerilizedTable::Mark(std::u32string_view string, bool greey) const
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
			SEEdgeDescription const* current_edge;
			size_t current_edge_index;
			size_t edge_count;
			std::u32string_view last_string;
			size_t capture_count;
		};
		struct NextSearch
		{
			size_t node;
			std::u32string_view last;
		};
		std::deque<CaptureTuple> capture_stack;
		std::deque<SearchStack> search_stack;
		search_stack.push_back({0, EdgeStart(0), 0, Node(0)->edge_count, string, 0});
		std::optional<March> last_acception;
		while (!search_stack.empty())
		{
			const SearchStack stack = *search_stack.rbegin();
			search_stack.pop_back();
			capture_stack.resize(stack.capture_count);
			SEEdgeDescription const* next_edge = nullptr;
			std::optional<NextSearch> next_search;
			switch (stack.current_edge->type)
			{
			case SEdgeType::Comsume:
			{
				SEComsume const* comsume = reinterpret_cast<SEComsume const*>(stack.current_edge + 1);
				size_t seg_count = comsume->count;
				Segment const* seg = reinterpret_cast<Segment const*>(comsume + 1);
				next_edge = reinterpret_cast<SEEdgeDescription const*>(seg + seg_count);
				if (!stack.last_string.empty())
				{
					IntervalWrapper wrapper(seg, seg_count);
					auto cur_character = *stack.last_string.begin();
					if (wrapper.IsInclude(cur_character))
						next_search = NextSearch{ stack.current_edge->jump_state, {stack.last_string.data() + 1, stack.last_string.size() - 1} };
				}
			}break;
			case SEdgeType::Acception: {
				SEAcception const* acception = reinterpret_cast<SEAcception const*>(stack.current_edge + 1);
				search_stack.clear();
				std::u32string_view capture_str(string.data(), string.size() - stack.last_string.size());
				if (greey)
				{
					next_edge = reinterpret_cast<SEEdgeDescription const*>(acception + 1);
					if (!last_acception || last_acception->capture.string.size() < capture_str.size())
					{
						last_acception = March{
							{capture_str, 0},
							acception->acception_index,
							acception->acception_mask,
							{}
						};
					}
					next_search = NextSearch{ stack.current_edge->jump_state, stack.last_string };
				}
				else
				{
					last_acception = March{
							{capture_str, 0},
							acception->acception_index,
							acception->acception_mask,
							{}
					};
				}
			} break;
			case SEdgeType::Capture:
			{
				SECapture const* capture = reinterpret_cast<SECapture const*>(stack.current_edge + 1);
				next_edge = reinterpret_cast<SEEdgeDescription const*>(capture + 1);
				capture_stack.push_back({ capture->begin == 1, string.size() - stack.last_string.size(), capture->require_index });
				next_search = NextSearch{ stack.current_edge->jump_state, stack.last_string };
			}
			case SEdgeType::Epsilon:
			{
				next_edge = stack.current_edge + 1;
				next_search = NextSearch{ stack.current_edge->jump_state, stack.last_string };
			}break;
			default: {assert(false); } break;
			}
			if(next_edge != nullptr)
			{
				auto old_stack = stack;
				++old_stack.current_edge_index;
				if(old_stack.current_edge_index < old_stack.edge_count)
				{
					old_stack.current_edge = next_edge;
					search_stack.push_back(old_stack);
				}
			}
			if(next_search)
			{
				SearchStack stack{
					next_search->node,
					EdgeStart(next_search->node),
					0, Node(next_search->node)->edge_count,
					next_search->last,
					capture_stack.size()
				};
				if(stack.current_edge_index < stack.edge_count)
					search_stack.push_back(stack);
			}
		}
		if (last_acception)
		{
			std::vector<CaptureTuple> handle_stack;
			for (auto& ite : capture_stack)
			{
				if (ite.require_state == last_acception->acception_state)
				{
					if (!ite.is_begin)
					{
						assert(!handle_stack.empty() && handle_stack.rbegin()->is_begin);
						auto start_index = handle_stack.rbegin()->capture_index;
						last_acception->sub_capture.emplace_back(March::Sub{ std::u32string_view{string.data() + start_index, string.data() + ite.capture_index}, start_index });
						handle_stack.pop_back();
					}
					else
						handle_stack.push_back(ite);
				}
			}
		}
		return last_acception;
	}
}
