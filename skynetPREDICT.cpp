#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <fstream>

#include <boost/tokenizer.hpp>
#include <Eigen/Dense>
#include "RNN.h"

using namespace std;
using namespace Eigen;
using namespace boost;


void print(auto s);

// cell is something like: 13.01.2017 23:49:00.000
vector<double> parse_date(string &s){
	typedef boost::tokenizer<boost::char_separator<char>> 
	tok;
	boost::char_separator<char> sep(".: ");
	tok tokens(s, sep);

	vector<double> dates_tok;
	for (auto it = tokens.begin(); it != tokens.end(); ++it){
		dates_tok.push_back(stod(*it));
	}

	return dates_tok;
}

/*
returns matrix with cols:
DD
MM
YYYY
HH
MM
[price_i,vol_i,[features_i]*]*
*/

template<typename Mat>
Mat getCSVData(const std::string &csvPath){
	ifstream inpData;
	inpData.open(csvPath);
	string line;
	vector<double> vals;
	uint rows = 0;

	getline(inpData, line); // skip first line
	while (getline(inpData, line)){
		stringstream lStream(line);
		string cell;

		// first line will be date
		getline(lStream, cell, ',');
		vector<double> date = parse_date(cell); // len(date) = 7

		// -2 because we don't need MS or S times, will always be 0
		vals.reserve(vals.size() + 
					distance(date.begin(), date.end() - 2));
		vals.insert(end(vals), begin(date), end(date) - 2);

		while (getline(lStream, cell, ','))
			vals.push_back(stod(cell));
		++rows;
	}

	return Map<
				const Matrix<
						typename Mat::Scalar, 
						Mat::RowsAtCompileTime, 
						Mat::ColsAtCompileTime, 
						RowMajor
						>
				>(vals.data(), rows, vals.size()/rows);
}

// for testing...
void print(auto s){
	cout << s << endl;
}

int main(int argc, char const *argv[])
{
	string version = "trading_hours";
	string pre_stock = "USA500.IDX";
	string freq = "1_m"; // 1 minute bars
	string bidask = "BID"; // which price to display
	string date_from = "09.01.2017";
	string date_to = "13.01.2017";

	auto fname = [](string s0, string s1, string s2, string s3, 
				string s4, string s5){
		return s0 + "_" + s1 + "_Candlestick_" + s2 
		+ "_" + s3 + "_" + s4 + "-" + s5 + ".csv";
	};

	string csv_name = "resources/PRICES/" + 
						fname(version, pre_stock, freq, 
						bidask, date_from, date_to);

	MatrixXd data = getCSVData<MatrixXd>(csv_name);

	int totalRows = data.rows();
	int trainRows = data.rows() * 0.75;
	int testRows = data.rows() * 0.25;

	MatrixXd x_train = data.topRows(trainRows).middleCols(5, 1);
	MatrixXd y_train = data.topRows(trainRows).col(7);

	MatrixXd x_test = data.bottomRows(testRows).middleCols(5, 1);
	MatrixXd y_test = data.bottomRows(testRows).col(7);

	bool PREDICT = true;
	
	RNN r(data.middleCols(5, 1), 30);
	r.normalize(x_train);
	r.normalize(y_train);
	r.normalize(x_test);

	// x, y, alpha, epoch, check_loss
	r.trainSGD(x_train, y_train, 0.000000001, 50, 1);
	if (PREIDCT){
		MatrixXd p = r.predict(x_test);
		r.denormalize(p);
		print("--------------------------------");
		print(p);
		print("--------------------------------");
	}

	return 0;
}