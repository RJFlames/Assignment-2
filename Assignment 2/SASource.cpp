#include <iostream>
#include <fstream>
#include <string>
#include <cctype>
#include <vector>
#include <algorithm>
#include <iomanip>

const std::vector<std::string> keywords = { "int", "float", "return", "function", "while", "if", "fi", "put" , "Boolean" , "real" , "get" , "true" , "false" };
const std::vector<std::string> seps = { "[", "]", "(", ")", "{", "}", ";", ":"  , "$$" , ",", "$" };
const std::vector<std::string> ops = { "=", "!", "<", ">", "-", "+", "*", "/" , "<=", ">=", "!=", "-=", "+=" };

class record {
private:
	std::string token, lexeme;
public:
	std::string getToken() { return this->token; }
	std::string getLexeme() { return this->lexeme; }

	void setLexeme(std::string s) { this->lexeme = s; }
	void setToken(std::string s) { this->token = s; }
};

bool FSM(std::string& state, char input, std::string& lexeme) {
	std::string c;
	c.push_back(input);
	if (state == "start") {
		if (isalpha(input)) { state = "identifier"; }
		else if (isdigit(input)) { state = "int"; }
		else if (input == '.') { state = "dot"; }
		else if (std::find(ops.begin(), ops.end(), c) != ops.end()) { state = "operator"; }
		else if (std::find(seps.begin(), seps.end(), c) != seps.end()) { state = "separator"; }
		else if (input == EOF) {
			state = "fileend";
			return true;
		}
	}

	else if (state == "identifier" && !isalnum(input) && input != '_') {
		return true;
	}
	else if (state == "int") {
		if (input == '.') { state = "dot"; }
		else if (!isdigit(input)) {
			return true;
		}
	}
	else if (state == "dot") {
		if (isdigit(input)) {
			state = "real";
		}
		else {
			state = "int";
			return true;
		}
	}
	else if (state == "real" && !isdigit(input)) {
		return true;
	}
	else if (state == "operator" && std::find(ops.begin(), ops.end(), lexeme + c) == ops.end()) {
		return true;
	}
	else if (state == "separator" && std::find(seps.begin(), seps.end(), lexeme + c) == seps.end()) {
		return true;
	}

	return false;
}

record callLexer(std::ofstream& out,std::ifstream& source) {
	std::string state = "start", lexeme = "";
	int done = 0;
	char c;
	while (done != 1) {
		c = source.get();

		if (FSM(state, c, lexeme) == true) {
			done = 1;
			source.unget();
		}

		if (state != "comments" && lexeme == "/*") {
			state = "comments";
			lexeme = "";
		}
		else if (state == "comments" && c == '*' && (c = source.get()) == '/') {
			state = "start";
			c = source.get();
		}

		if (done == 1) {
			if (state == "identifier" && std::find(keywords.begin(), keywords.end(), lexeme) != keywords.end()) { state = "keyword"; }
			record latest;
			latest.setLexeme(lexeme);
			latest.setToken(state);
			return latest;
		}

		if (!isspace(c) && state != "comments") { lexeme.push_back(c); }
	}
	
}

bool display = false;

void Syntax_Error(record latest, std::ofstream& out, std::string expected) {
	std::cerr << "Syntax Error: Expected " << expected << "\n";
	std::cerr << "Received " << latest.getToken() << " \"" << latest.getLexeme() << "\"";
	// 
}
void Lexeme_Check(std::ofstream& out,std::ifstream& source, std::string lexeme) {
	record latest = callLexer(out,source);
	if (latest.getLexeme() != lexeme) {
		Syntax_Error(latest, out,lexeme);
	}
}

record IDs_Cont(std::ofstream& out, std::ifstream& source) {
	if (display)
		out << "<IDs>' ::= ,  <IDs>  |  <Empty>'\n";
	record latest = callLexer(out, source);
	if (latest.getLexeme() == ",")
		return IDs(out, source,callLexer(out,source));
	else
		return latest;
}

record IDs(std::ofstream& out, std::ifstream& source,record latest) {
	if (display)
		out << "<IDs> ::= <Identifier> <IDs>'\n";
	if (latest.getToken() != "identifier")
		Syntax_Error(latest, out, "an identifier");
	return IDs_Cont(out, source);
}

void Body(std::ofstream& out, std::ifstream& source) {
	if (display)
		out << "<Body> ::= { < Statement List> }\n";
	Lexeme_Check(out, source, "{");
	record latest = callLexer(out, source);
	State_List(out, source, latest);
	Lexeme_Check(out, source, "}");
}

record Qualifier(std::ofstream& out, std::ifstream& source,record latest) {
	if (display)
		out << "<Qualifier> ::= int | boolean | real\n";
	if (latest.getLexeme() == "int" || latest.getLexeme() == "boolean" || latest.getLexeme() == "real")
		return callLexer(out, source);
	else
		Syntax_Error(latest, out, "int, boolean, or real");
}

void Parameter(std::ofstream& out, std::ifstream& source,record a) {
	if (display)
		out << "<Parameter> ::= <IDs> <Qualifier>\n";
	record latest = IDs(out, source, a);
	Qualifier(out, source,latest);
}

record Decla(std::ofstream& out, std::ifstream& source, record a) {
	if (display)
		out << "<Parameter> ::= <Qualifier> <IDs>\n";
	Qualifier(out, source, a);
	record latest = callLexer(out,source);
	return IDs(out, source, latest);
}

record Para_List_Cont(std::ofstream& out, std::ifstream& source) {
	if (display)
		out << "<Parameter List>\' ::= ,  <Parameter List>  |  <Empty>\n";
	record latest = callLexer(out,source);
	if (latest.getLexeme() == ",")
		return Para_List(out, source, callLexer(out,source));
	else
		return latest;
}

record Para_List(std::ofstream& out, std::ifstream& source, record latest) {
	if (display)
		out << "<Parameter List> ::= <Parameter> <Parameter List>'\n";
	Parameter(out, source,latest);
	return Para_List_Cont(out, source);
}

record Decla_List_Cont(std::ofstream& out, std::ifstream& source, record latest) {
	if (display)
		out << "<Declaration List>\' ::= <Declaration List>  |  <Empty>\n";
	if (latest.getLexeme() == "int" || latest.getLexeme() == "boolean" || latest.getLexeme() == "real")
		return Decla_List(out, source,latest);
	else
		return latest;
}

record Decla_List(std::ofstream& out, std::ifstream& source, record latest) {
	if (display)
		out << "<Declaration List> ::= <Declaration>  <Declaration List>\'\n";
	record l = Decla(out, source, latest);
	return Decla_List_Cont(out, source, l);
}

record State_List_Cont(std::ofstream& out, std::ifstream& source, record latest) {
	if (display)
		out << "<Statement List>\' ::= <Statement List>  |  <Empty>\n";
	if (latest.getLexeme() == "int" || latest.getLexeme() == "boolean" || latest.getLexeme() == "real")
		return State_List(out, source, latest);
	else
		return latest;
}

record State_List(std::ofstream& out, std::ifstream& source, record latest) {
	if (display)
		out << "<Statement List> ::= <Statement> <Statement  List>\'\n";
	latest = Statement(out, source, latest);
	return State_List_Cont(out, source,latest);
}

record OPL(std::ofstream& out, std::ifstream& source) {
	if (display)
		out << "<Opt Parameter List> ::= <Parameter List> | <Empty>\n";
	record latest = callLexer(out, source);
	if (latest.getToken() != "identifier") {
		return latest;
	}
	return Para_List(out, source, latest);
	
}

record ODL(std::ofstream& out, std::ifstream& source) {
	if (display)
		out << "<Opt Declaration List> ::= <Declaration List> | <Empty>\n";
	record latest = callLexer(out, source);
	if (latest.getLexeme() == "int" || latest.getLexeme() == "boolean" || latest.getLexeme() == "real") {
		return Decla_List(out, source,latest);
	}
	else
		return latest;
	
}

record Func(std::ofstream& out, std::ifstream& source) {
	if (display)
		out << "<Function> ::= function <Identifier> ( <Opt Parameter List> ) <Opt Declaration List> <Body>\n";

	record latest = callLexer(out, source);
	latest = IDs(out, source,latest);

	if (latest.getLexeme() == "(") {
		Syntax_Error(latest, out, "(");
	}
	latest = OFD(out, source);
	if (latest.getLexeme() == ")") {
		Syntax_Error(latest, out , ")");
	}
	latest = ODL(out, source);
	
	if (latest.getLexeme() == "{") {
		Body(out, source);
	}
	else
		Syntax_Error(latest, out, "{");
	
}

record Func_Def_Cont(std::ofstream& out, std::ifstream& source) {
	if (display)
		out << "<Function Definitions>' ::= <Function Definitions> | <Empty>\n";
	record latest = callLexer(out, source);
	if (latest.getLexeme() != "function") {
		return latest;
	}
	else
		return Func_Def(out, source);
}

record Func_Def(std::ofstream& out, std::ifstream& source) {
	if (display)
		out << "<Function Definitions> ::= <Function> <Function Definitions>'\n";
	Func(out, source);
	return Func_Def_Cont(out, source);
}
record OFD(std::ofstream& out, std::ifstream& source) {

	if (display)
		out << "<Opt Function Definitions> ::= <Function Definitions> | <Empty>\n";
	record latest = callLexer(out, source);
	if (latest.getLexeme() != "function") {
		return latest;
	}
	else
		return Func_Def(out, source);
}

void Rat20F(std::ofstream& out, std::ifstream& source) {
	if (display)
		out << "<Rat20F> ::= <Opt Function Definitions> $$ <Opt Declaration List> <Statement List> $$\n";

	record latest = OFD(out,source);

	if (latest.getLexeme() != "$$")
		Syntax_Error(latest, out,"$$");

	latest = ODL(out,source); // <Opt Declaration List>
	State_List(out, source, latest);
	Lexeme_Check(out, source, "$$");
}

int main(int argc, const char* argv[]) {

	std::ifstream source(argv[1]);
	std::ofstream out(argv[2]);

	Rat20F(out, source);

	out.close();
	source.close();
	return 0;
}
