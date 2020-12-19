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
			uint32_t accepetable_state;
			uint32_t accepetable_mask;
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
		uint32_t acception_state;
		uint32_t acception_mask;
		std::vector<Sub> sub_capture;
	};
	
	struct Table
	{

		struct EAcception{ uint32_t acception_index; uint32_t acception_mask; };
		struct EEpsilon{};
		struct ECapture{ uint32_t begin; uint32_t require_index; };
		struct EComsume { Interval interval; };
		
		struct Edge
		{
			using PropertyT = std::variant<EAcception, EEpsilon, ECapture, EComsume>;
			uint32_t jump_state;
			PropertyT property;
			Edge(Edge const&) = default;
			Edge(Edge&&) = default;
			Edge(uint32_t js, PropertyT T) : jump_state(js), property(std::move(T)){}
			template<typename Type>
			bool Is() const noexcept {return std::holds_alternative<Type>(property); }
			template<typename Type>
			decltype(auto) Get() const noexcept { return std::get<Type>(property); }
			template<typename Type>
			decltype(auto) Get() noexcept { return std::get<Type>(property); }
			Edge& operator=(Edge&&) = default;
			Edge& operator=(Edge const&) = default;
		};
		std::vector<std::vector<Edge>> nodes;
		
		static Table CreateFromRegex(std::u32string_view rex, uint32_t state = 0, uint32_t mask = 0);
		size_t NodeCount() const noexcept {return nodes.size(); }
		size_t StartNodeIndex() const noexcept {return 0;}
		//std::optional<March> Mark(std::u32string_view string, bool greey = true) const;
		std::tuple<std::set<uint32_t>, std::vector<Edge>> SearchThroughEpsilonEdge(uint32_t const* require_state, size_t length) const;
		std::vector<std::tuple<Interval, std::vector<uint32_t>>> MergeComsumeEdge(Edge const* edges, size_t edges_length) const;
		static Table Link(Table const* other_table, size_t table_size);
		static void DefaultFilter(Table const&, std::vector<Edge>&);
		Table Simplify(std::function<void(Table const&, std::vector<Edge>&)> edge_filter = Table::DefaultFilter) const;
		operator bool() const noexcept{return nodes.size() >= 2;}
	};

	inline Table CreateUnfaTableFromRegex(std::u32string_view rex, uint32_t state = 0, uint32_t mask = 0){ return Table::CreateFromRegex(rex, state, mask); }
	inline Table LinkUnfaTable(Table const* other_table, size_t table_size) { return Table::Link(other_table, table_size); }
	
	struct SerilizedTable
	{

		enum class SEdgeType : uint32_t
		{
			Acception,
			Epsilon,
			Capture,
			Comsume
		};

		struct SEEdgeDescription
		{
			SEdgeType type;
			uint32_t jump_state;
		};

		struct SENode { uint32_t edge_start_offset; uint32_t edge_count; };

		size_t NodeCount() const noexcept { return node_count; }
		size_t StartNodeIndex() const noexcept { return 0; }
		
		SENode const* Node(size_t node_index) const noexcept{ assert(node_count >= node_index); return reinterpret_cast<SENode const*>(datas.data()) + node_index; }
		SEEdgeDescription const* EdgeStart(size_t node_index) const noexcept
		{
			assert(node_count >= node_index);
			return reinterpret_cast<SEEdgeDescription const*>(Node(node_index)->edge_start_offset * sizeof(uint32_t) + datas.data());
		}
		
		operator bool() const noexcept { return !datas.empty() && node_count >= 2; }

		SerilizedTable() = default;
		SerilizedTable(Table const& table);
		SerilizedTable(SerilizedTable const&) = default;
		SerilizedTable(SerilizedTable&&) = default;
		SerilizedTable& operator=(SerilizedTable const&) = default;
		SerilizedTable& operator=(SerilizedTable&&) = default;
		std::optional<March> Mark(std::u32string_view string, bool greey = true) const;
		
	private:
		std::vector<std::byte> datas;
		size_t node_count = 0;
		
	};

	inline SerilizedTable Serilized(Table const& table) { return {table};  }

}