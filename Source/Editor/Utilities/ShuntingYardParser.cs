// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using FlaxEngine;

namespace FlaxEditor.Utilities
{
    /// <summary>
    /// The Shunting-Yard algorithm parsing utilities.
    /// </summary>
    [HideInEditor]
    public static class ShuntingYard
    {
        /// <summary>
        /// The Shunting-Yard parser exception type.
        /// </summary>
        /// <seealso cref="System.Exception" />
        public class ParsingException : Exception
        {
            /// <summary>
            /// Initializes a new instance of the <see cref="ParsingException"/> class.
            /// </summary>
            /// <param name="msg">The message.</param>
            public ParsingException(string msg)
            : base("Parsing error: " + msg)
            {
            }
        }

        /// <summary>
        /// Types of possible tokens used in Shunting-Yard parser.
        /// </summary>
        [HideInEditor]
        public enum TokenType
        {
            /// <summary>
            /// The number.
            /// </summary>
            Number,

            /// <summary>
            /// The variable.
            /// </summary>
            Variable,

            /// <summary>
            /// The parenthesis.
            /// </summary>
            Parenthesis,

            /// <summary>
            /// The operator.
            /// </summary>
            Operator,

            /// <summary>
            /// The white space character.
            /// </summary>
            WhiteSpace,
        };

        /// <summary>
        /// Token representation containing its type and value.
        /// </summary>
        public struct Token
        {
            /// <summary>
            /// Gets the token type.
            /// </summary>
            public TokenType Type;

            /// <summary>
            /// Gets the token value.
            /// </summary>
            public string Value;

            /// <summary>
            /// Initializes a new instance of the <see cref="Token"/> struct.
            /// </summary>
            /// <param name="type">The type.</param>
            /// <param name="value">The value.</param>
            public Token(TokenType type, string value)
            {
                Type = type;
                Value = value;
            }
        }

        /// <summary>
        /// Represents simple mathematical operation.
        /// </summary>
        private class Operator
        {
            public string Name;
            public int Precedence;
            public bool RightAssociative;
        }

        /// <summary>
        /// Describe all operators available for parsing.
        /// </summary>
        private static readonly IDictionary<string, Operator> Operators = new Dictionary<string, Operator>
        {
            ["+"] = new Operator { Name = "+", Precedence = 1 },
            ["-"] = new Operator { Name = "-", Precedence = 1 },
            ["*"] = new Operator { Name = "*", Precedence = 2 },
            ["/"] = new Operator { Name = "/", Precedence = 2 },
            ["^"] = new Operator { Name = "^", Precedence = 3, RightAssociative = true },
        };

        /// <summary>
        /// Describe all predefined variables for parsing (all lowercase).
        /// </summary>
        private static readonly IDictionary<string, double> Variables = new Dictionary<string, double>
        {
            ["pi"] = Math.PI,
            ["e"] = Math.E,
            ["infinity"] = double.MaxValue,
            ["-infinity"] = -double.MaxValue,
            ["m"] = Units.Meters2Units,
            ["cm"] = Units.Meters2Units / 100,
            ["km"] = Units.Meters2Units * 1000,
            ["s"] = 1,
            ["ms"] = 0.001,
            ["min"] = 60,
            ["h"] = 3600,
            ["cm²"] = (Units.Meters2Units / 100) * (Units.Meters2Units / 100),
            ["cm³"] = (Units.Meters2Units / 100) * (Units.Meters2Units / 100) * (Units.Meters2Units / 100),
            ["dm²"] = (Units.Meters2Units / 10) * (Units.Meters2Units / 10),
            ["dm³"] = (Units.Meters2Units / 10) * (Units.Meters2Units / 10) * (Units.Meters2Units / 10),
            ["l"] = (Units.Meters2Units / 10) * (Units.Meters2Units / 10) * (Units.Meters2Units / 10),
            ["m²"] = Units.Meters2Units * Units.Meters2Units,
            ["m³"] = Units.Meters2Units * Units.Meters2Units * Units.Meters2Units,
            ["kg"] = 1,
            ["g"] = 0.001,
            ["n"] = Units.Meters2Units
        };

        /// <summary>
        /// List known units which cannot be handled as a variable easily because they contain operator symbols (mostly a forward slash). The value is the factor to calculate game units. 
        /// </summary>
        private static readonly IDictionary<string, double> UnitSymbols = new Dictionary<string, double>
        {
            ["cm/s"] = Units.Meters2Units / 100,
            ["cm/s²"] = Units.Meters2Units / 100,
            ["m/s"] = Units.Meters2Units,
            ["m/s²"] = Units.Meters2Units,
            ["km/h"] = 1 / 3.6 * Units.Meters2Units,
            // Nm is here because these values are compared case-sensitive, and we don't want to confuse nanometers and Newtonmeters
            ["Nm"] = Units.Meters2Units * Units.Meters2Units,
        };

        /// <summary>
        /// Compare operators based on precedence: ^ >> * / >> + -
        /// </summary>
        /// <param name="oper1">The first operator.</param>
        /// <param name="oper2">The second operator.</param>
        /// <returns>The comparison result.</returns>
        private static bool CompareOperators(string oper1, string oper2)
        {
            var op1 = Operators[oper1];
            var op2 = Operators[oper2];
            return op1.RightAssociative ? op1.Precedence < op2.Precedence : op1.Precedence <= op2.Precedence;
        }

        /// <summary>
        /// Assign a single character to its TokenType.
        /// </summary>
        /// <param name="c">The input character.</param>
        /// <returns>The token type.</returns>
        private static TokenType DetermineType(char c)
        {
            if (char.IsDigit(c) || c == '.')
                return TokenType.Number;

            if (char.IsWhiteSpace(c))
                return TokenType.WhiteSpace;

            if (c == '(' || c == ')')
                return TokenType.Parenthesis;

            var str = char.ToString(c);
            if (Operators.ContainsKey(str))
                return TokenType.Operator;

            if (char.IsLetter(c) || c == '²' || c == '³')
                return TokenType.Variable;

            throw new ParsingException("wrong character");
        }

        /// <summary>
        /// First parsing step - tokenization of a string.
        /// </summary>
        /// <param name="text">The input text.</param>
        /// <returns>The collection of parsed tokens.</returns>
        public static IEnumerable<Token> Tokenize(string text)
        {
            // Prepare text
            text = text.Replace(',', '.').Replace("°", "");
            foreach (var kv in UnitSymbols)
            {
                int idx;
                do
                {
                    idx = text.IndexOf(kv.Key, StringComparison.InvariantCulture);
                    if (idx > 0)
                    {
                        if (DetermineType(text[idx - 1]) != TokenType.Number)
                            throw new ParsingException($"unit found without a number: {kv.Key} at {idx} in {text}");
                        if (Mathf.Abs(kv.Value - 1) < Mathf.Epsilon)
                            text = text.Remove(idx, kv.Key.Length);
                        else
                            text = text.Replace(kv.Key, "*" + kv.Value);
                    }
                } while (idx > 0);
            }

            // Necessary to correctly parse negative numbers
            var previous = TokenType.WhiteSpace;

            var token = new StringBuilder();
            for (int i = 0; i < text.Length; i++)
            {
                var type = DetermineType(text[i]);
                if (type == TokenType.WhiteSpace)
                    continue;

                token.Clear();
                token.Append(text[i]);

                // Handle fractions and negative numbers (dot . is considered a figure)
                if (type == TokenType.Number || text[i] == '-' && previous != TokenType.Number)
                {
                    // Parse the whole number till the end
                    if (i + 1 < text.Length)
                    {
                        switch (text[i + 1])
                        {
                        case 'x':
                        case 'X':
                        {
                            // Hexadecimal value
                            i++;
                            token.Clear();
                            if (i + 1 == text.Length || !StringUtils.IsHexDigit(text[i + 1]))
                                throw new ParsingException("invalid hexadecimal number");
                            while (i + 1 < text.Length && StringUtils.IsHexDigit(text[i + 1]))
                            {
                                token.Append(text[++i]);
                            }
                            var value = ulong.Parse(token.ToString(), NumberStyles.HexNumber);
                            token.Clear();
                            token.Append(value.ToString());
                            break;
                        }
                        default:
                        {
                            // Decimal value
                            while (i + 1 < text.Length && DetermineType(text[i + 1]) == TokenType.Number)
                            {
                                token.Append(text[++i]);
                            }

                            // Exponential notation
                            if (i + 2 < text.Length && (text[i + 1] == 'e' || text[i + 1] == 'E'))
                            {
                                token.Append(text[++i]);
                                if (text[i + 1] == '-' || text[i + 1] == '+')
                                    token.Append(text[++i]);
                                while (i + 1 < text.Length && DetermineType(text[i + 1]) == TokenType.Number)
                                {
                                    token.Append(text[++i]);
                                }
                            }
                            break;
                        }
                        }
                    }

                    // Discard solo '-'
                    if (token.Length != 1)
                        type = TokenType.Number;
                }
                else if (type == TokenType.Variable)
                {
                    if (previous == TokenType.Number)
                    {
                        previous = TokenType.Operator;
                        yield return new Token(TokenType.Operator, "*");
                    }
                    // Continue till the end of the variable
                    while (i + 1 < text.Length && DetermineType(text[i + 1]) == TokenType.Variable)
                    {
                        i++;
                        token.Append(text[i]);
                    }
                }

                previous = type;
                yield return new Token(type, token.ToString());
            }
        }

        /// <summary>
        /// Second parsing step - order tokens in reverse polish notation.
        /// </summary>
        /// <param name="tokens">The input tokens collection.</param>
        /// <returns>The collection of the tokens in reverse polish notation order.</returns>
        public static IEnumerable<Token> OrderTokens(IEnumerable<Token> tokens)
        {
            var stack = new Stack<Token>();

            foreach (var tok in tokens)
            {
                switch (tok.Type)
                {
                // Number and variable tokens go directly to output
                case TokenType.Variable:
                case TokenType.Number:
                    yield return tok;
                    break;

                // Operators go on stack, unless last operator on stack has higher precedence
                case TokenType.Operator:
                    while (stack.Any() && stack.Peek().Type == TokenType.Operator && CompareOperators(tok.Value, stack.Peek().Value))
                    {
                        yield return stack.Pop();
                    }

                    stack.Push(tok);
                    break;

                // Left parenthesis goes on stack
                // Right parenthesis takes all symbols (...) to output
                case TokenType.Parenthesis:
                    if (tok.Value == "(")
                        stack.Push(tok);
                    else
                    {
                        while (stack.Peek().Value != "(")
                            yield return stack.Pop();
                        stack.Pop();
                    }
                    break;

                default: throw new ParsingException("wrong token");
                }
            }

            // Pop all remaining operators from stack
            while (stack.Any())
            {
                var tok = stack.Pop();

                // Parenthesis still on stack mean a mismatch
                if (tok.Type == TokenType.Parenthesis)
                    throw new ParsingException("mismatched parentheses");

                yield return tok;
            }
        }

        /// <summary>
        /// Third parsing step - evaluate reverse polish notation into single float.
        /// </summary>
        /// <param name="tokens">The input token collection.</param>
        /// <returns>The result value.</returns>
        public static double EvaluateRPN(IEnumerable<Token> tokens)
        {
            var stack = new Stack<double>();

            foreach (var token in tokens)
            {
                if (token.Type == TokenType.Number)
                {
                    stack.Push(double.Parse(token.Value, NumberStyles.Float | NumberStyles.AllowThousands, CultureInfo.InvariantCulture));
                }
                else if (token.Type == TokenType.Variable)
                {
                    if (Variables.TryGetValue(token.Value.ToLower(), out double variableValue))
                    {
                        stack.Push(variableValue);
                    }
                    else
                    {
                        throw new ParsingException($"unknown variable : {token.Value}");
                    }
                }
                else
                {
                    // In this step there always should be 2 values on stack
                    if (stack.Count < 2)
                        throw new ParsingException("evaluation error");

                    var f1 = stack.Pop();
                    var f2 = stack.Pop();

                    switch (token.Value)
                    {
                    case "+":
                        f2 += f1;
                        break;
                    case "-":
                        f2 -= f1;
                        break;
                    case "*":
                        f2 *= f1;
                        break;
                    case "/":
                        if (Math.Abs(f1) < 1e-7f)
                            throw new Exception("division by 0");
                        f2 /= f1;
                        break;
                    case "^":
                        f2 = Math.Pow(f2, f1);
                        break;
                    }

                    stack.Push(f2);
                }
            }

            // if stack has more than one item we're not finished with evaluating
            // we assume the remaining values are all factors to be multiplied
            if (stack.Count > 1)
            {
                var v1 = stack.Pop();
                while (stack.Count > 0)
                    v1 *= stack.Pop();
                return v1;
            }
            return stack.Pop();
        }

        /// <summary>
        /// Parses the specified text and performs the Shunting-Yard algorithm execution.
        /// </summary>
        /// <param name="text">The input text.</param>
        /// <returns>The result value.</returns>
        public static double Parse(string text)
        {
            var tokens = Tokenize(text);
            var rpn = OrderTokens(tokens);
            return EvaluateRPN(rpn);
        }
    }
}
