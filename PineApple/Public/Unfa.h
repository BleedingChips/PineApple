#pragma once
#include "Interval.h"
#include <string_view>
#include <variant>
#include <optional>
#include <set>
#include <functional>

namespace PineApple::Unfa
{
	using Interval = ::PineApple::Interval<char32_t>;
	using IntervalWrapper = ::PineApple::IntervalWrapper<char32_t>;
	using Segment = typename Interval::Segment;

	namespace Error
	{
		struct UnaccaptableRexgex {
			std::u32string regex;
			size_t accepetable_state;
			size_t accepetable_mask;
			size_t Index;
		};
	}

	struct March
	{
		struct Sub
		{
			std::u32string_view string;
			size_t index;
		};
		Sub capture;
		size_t acception_state;
		size_t acception_mask;
		std::vector<Sub> sub_capture;
	};
	
	struct Table
	{
		enum class EdgeType : size_t
		{
			Acception,
			Comsume,
			Epsilon,
			Capture,
		};

		struct Edge
		{
			union 
			{
				struct { EdgeType type; size_t jump_state, s1, s2;};
				struct { EdgeType type; size_t jump_state, acception_state, acception_mask; } acception;
				struct { EdgeType type; size_t jump_state, character_set_start_index, character_set_count; } comsume;
				struct { EdgeType type; size_t jump_state; } epsilon;
				struct { EdgeType type; size_t jump_state, is_begin, require_state; } capture;
			};
			Edge(EdgeType input_type, size_t input_jump_state, size_t v1 = 0, size_t v2 = 0) :
				type(input_type), jump_state(input_jump_state), s1(v1), s2(v2)
			{
				acception.acception_state = v1;
				acception.acception_mask = v2;
			}
			Edge(size_t input_type, size_t input_jump_state, size_t v1 = 0, size_t v2 = 0) :
				Edge(static_cast<EdgeType>(input_type), input_jump_state, v1, v2){}
			Edge(Edge const&) = default;
			Edge& operator=(Edge const&) = default;
		};

		struct Node
		{
			size_t edge_start_index;
			size_t edge_count;
		};
		
		std::vector<Segment> character_set;
		std::vector<Edge> edges;
		std::vector<Node> nodes;

		static Table CreateFromRegex(std::u32string_view rex, size_t state = 0, size_t mask = 0);
		size_t NodeCount() const noexcept {return nodes.size(); }
		size_t StartNodeIndex() const noexcept {return 0;}
		std::optional<March> Mark(std::u32string_view string, bool greey = true) const;
		std::tuple<std::set<size_t>, std::vector<Edge>> SearchThroughEpsilonEdge(size_t const* require_state, size_t length) const;
		std::vector<std::tuple<Interval, std::vector<size_t>>> MergeComsumeEdge(Edge const* edges, size_t edges_length) const;
		static Table Link(Table const* other_table, size_t table_size);
		static void DefaultFilter(Table const&, std::vector<Edge>&);
		Table Simplify(std::function<void(Table const&, std::vector<Edge>&)> edge_filter = Table::DefaultFilter) const;
		operator bool() const noexcept{return nodes.size() >= 2;}
	};

	inline Table CreateUnfaTableFromRegex(std::u32string_view rex, size_t state = 0, size_t mask = 0){ return Table::CreateFromRegex(rex, state, mask); }
	inline Table LinkUnfaTable(Table const* other_table, size_t table_size) { return Table::Link(other_table, table_size); }

	struct DebuggerTable
	{
		DebuggerTable(Table const& table);
		struct Edge
		{
			Table::Edge edge;
			Interval comsume_interval;
		};
		struct Node
		{
			std::vector<Edge> edges;
		};
		std::vector<Node> nodes;
	};

}