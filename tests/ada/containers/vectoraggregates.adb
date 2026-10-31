with Ada.Containers; use Ada.Containers;
with Ada.Containers.Vectors;
with Ada.Containers.Indefinite_Vectors;
with Ada.Strings.Unbounded; use Ada.Strings.Unbounded;
with Ada.Text_IO;

procedure VectorAggregates is
    type Token_Kind is (T_Number, T_Ident);
    type Token is record
        Kind : Token_Kind;
        Text : Unbounded_String;
        Value : Long_Float;
        Op : Character;
    end record;
    package Tokens is new Ada.Containers.Vectors (Natural, Token);
    package Indefinite_Tokens is new Ada.Containers.Indefinite_Vectors (Natural, Token);
    Result : Tokens.Vector;
    Other : Indefinite_Tokens.Vector;

    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error with "vector aggregate check";
        end if;
    end Check;
begin
    Result.Append ((Kind => T_Number, Text => Null_Unbounded_String,
                    Value => 12.5, Op => ' '));
    Result.Append ((Kind => T_Ident, Text => To_Unbounded_String ("name"),
                    Value => 0.0, Op => ' '));
    Tokens.Prepend (Result, (Kind => T_Number, Text => Null_Unbounded_String,
                            Value => 1.0, Op => '+'));
    Tokens.Insert (Result, Before => Natural'(1),
                   New_Item => (Kind => T_Number, Text => Null_Unbounded_String,
                                Value => 2.0, Op => '-'));
    Check (Result.Length = 4);
    Check (Result.Element (0).Value = 1.0);
    Check (Result.Element (1).Value = 2.0);
    Check (Result.Element (2).Value = 12.5);
    Check (To_String (Result.Element (3).Text) = "name");
    -- The vector-valued overload must still work as well.
    Result.Append (Result);
    Check (Result.Length = 8);
    Check (To_String (Result.Element (7).Text) = "name");

    Other.Append ((Kind => T_Number, Text => Null_Unbounded_String,
                   Value => 3.0, Op => ' '));
    Indefinite_Tokens.Append (Other,
                             (Kind => T_Ident, Text => To_Unbounded_String ("other"),
                              Value => 0.0, Op => ' '));
    Check (Other.Length = 2);
    Check (Other.Element (0).Value = 3.0);
    Check (To_String (Other.Element (1).Text) = "other");
    Ada.Text_IO.Put_Line ("vector aggregates ok");
end VectorAggregates;
