#include <algorithm>
#include <iostream>
#include <string>
#include <tabulate/column.hpp>
#include <tabulate/font_style.hpp>
#include <tabulate/printer.hpp>
#include <tabulate/row.hpp>
#include <tabulate/termcolor.hpp>
#include <vector>

namespace tabulate {

Format &Cell::format() {
  if (!format_.has_value()) { // no cell format
    std::shared_ptr<Row> parent = parent_.lock();
    format_ = parent->format(); // Use parent row format
  }
  return format_.value();
}

class TableInternal : public std::enable_shared_from_this<TableInternal> {
public:
  static std::shared_ptr<TableInternal> create() {
    auto result = std::shared_ptr<TableInternal>(new TableInternal());
    result->format_.set_defaults();
    return result;
  }

  void add_row(const std::vector<std::string> &cells) {
    auto row = std::make_shared<Row>(shared_from_this());
    for (auto &c : cells) {
      auto cell = std::make_shared<Cell>(row);
      cell->set_text(c);
      row->add_cell(cell);
    }
    rows_.push_back(row);
  }

  Row &operator[](size_t index) { return *(rows_[index]); }

  const Row &operator[](size_t index) const { return *(rows_[index]); }

  Column column(size_t index) {
    Column column(shared_from_this());
    for (size_t i = 0; i < rows_.size(); ++i) {
      auto row = rows_[i];
      auto &cell = row->cell(index);
      column.add_cell(cell);
    }
    return column;
  }

  size_t size() const { return rows_.size(); }

  Format &format() { return format_; }

  void print(std::ostream &stream) {
    Printer::print(*this, stream);
  }

  size_t estimate_num_columns() const {
    size_t result{0};
    if (size()) {
      auto first_row = operator[](size_t(0));
      result = first_row.size();
    }
    return result;
  }

private:
  TableInternal() {}
  TableInternal &operator=(const TableInternal &);
  TableInternal(const TableInternal &);

  std::vector<std::shared_ptr<Row>> rows_;
  Format format_;
};

Format &Row::format() {
  if (!format_.has_value()) { // no row format
    std::shared_ptr<TableInternal> parent = parent_.lock();
    format_ = parent->format(); // Use parent table format
  }
  return format_.value();
}

void Printer::print(TableInternal& table, std::ostream& stream) {
  size_t num_rows = table.size();
  size_t num_columns = table.estimate_num_columns();

  std::vector<size_t> row_heights, column_widths{};
  
  for (size_t i = 0; i < num_columns; ++i) {
    Column column = table.column(i);
    size_t configured_width = column.get_configured_width();
    size_t computed_width = column.get_computed_width();
    if (configured_width != 0)
      column_widths.push_back(configured_width);
    else
      column_widths.push_back(computed_width);
  }

  for(size_t i = 0; i < num_rows; ++i) {
    Row row = table[i];
    size_t configured_height = row.get_configured_height();
    size_t computed_height = row.get_computed_height(column_widths);

    // NOTE: Unlike column width, row height is calculated as the max 
    // b/w configured height and computed height
    // which means that .width() has higher precedence than .height()
    // when both are configured by the user
    // TODO: Maybe this can be configured?
    // If such a configuration is exposed, i.e., prefer height over width
    // then the logic will be reversed, i.e.,
    // column_widths.push_back(std::max(configured_width, computed_width)) 
    // and
    // row_height = configured_height if != 0 else computed_height
    //

    row_heights.push_back(std::max(configured_height, computed_height));
  }

  for (size_t i = 0; i < num_rows; ++i) {
    auto row_height = row_heights[i];
    for (size_t j = 0; j < num_columns; ++j) {
      auto column_width = column_widths[j];
      std::cout << row_height << "x" << column_width << " ";
    }
    std::cout << "\n";
  }

  std::cout << "\n";
}

} // namespace tabulate