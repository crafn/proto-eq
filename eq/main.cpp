#include "rel.hpp"
#include "var.hpp"

#include <iostream>

namespace gui {

class Box {
public:

	Box()
	{
		eq::rel(height() >= 0);
	}

	eq::Var<int>& top() { return top_; }
	eq::Var<int>& bottom() { return bottom_; }

	auto height() -> decltype(top() - bottom()) { return top() - bottom(); }

private:
	eq::Var<int> top_;
	eq::Var<int> bottom_;
};

} // gui

std::ostream& operator<< (std::ostream& stream, gui::Box& box)
{
	stream << "top: " << box.top() << ", bottom: " << box.bottom();
}

int main()
{
	{
		gui::Box box1, box2;
		rel(box1.top() == 30 && box2.bottom() == 0);
		rel(box1.bottom() == box2.top() && box1.height() == box2.height());

		std::cout << "box1 " << box1 << std::endl;
		std::cout << "box2 " << box2 << std::endl;
	}

	{
		eq::PriorityVar low, high;
		rel(high > low);

		eq::Var<int> x, y;
		
		rel(x > 0, low);
		rel(x < 10, high);
		rel(x*x == 12*12);
		rel(y == 2*x, low);

		std::cout << x << ", " << y << std::endl;
	}

	{
		eq::PriorityVar low, med, high;
		rel(low < med && med < high);

		eq::Var<int> x;
		rel(x == 1, high);
		rel(x == 2, med);
		rel(x == 3, low);

		eq::Var<int> y= std::move(x);
		std::cout << y << std::endl;
	}

	{
		eq::PriorityVar init;

		eq::Var<int> a, b;

		rel(b == 1337, init);
		rel(a == 1 && b == 2);
		
		a.clear();
		std::cout << b << std::endl; // 1337
	}
}
