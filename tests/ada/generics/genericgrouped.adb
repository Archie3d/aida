with Ada.Text_IO;
with Grouped_Model;
procedure GenericGrouped is
    procedure Check is
        Seed : Integer := 7;
        generic
            Seed, Other : Integer := Seed;
        package Shadow is
            First : Integer := Seed;
            Second : Integer := Other;
        end Shadow;
        package Shadow_Default is new Shadow;
        package Shadow_Actual is new Shadow (Seed => 10);
        generic
            Base : Integer;
        package Outer is
            function Value return Integer;
        end Outer;
        package body Outer is
            function Value return Integer is
                generic
                    Base, Copy : Integer := Base;
                package Inner is
                    Result : Integer := Base + Copy;
                end Inner;
                package Defaulted is new Inner;
                package Changed is new Inner (Base => 20);
            begin
                return Defaulted.Result + Changed.Result;
            end Value;
        end Outer;
        package Outer_One is new Outer (3);
        package Outer_Two is new Outer (9);
        Calls : Integer := 0;
        function Next return Integer is
        begin
            Calls := Calls + 1;
            return Calls;
        end Next;
        generic
            A, B, C : Integer := Next;
            D : Integer := A + B + C;
        package Values is
            Sum : Integer := D;
        end Values;
        package First is new Values;
        package Second is new Values (10, C => 30);
        package Third is new Values (C => 3, B => 2, A => 1);
        X : Integer := 0;
        Y : Integer := 10;
        package Text is new Grouped_Model (String, "abc", "abc", X, Y);
        package Other is new Grouped_Model
            (Extra => 5, Y => X, X => Y, Right => "x", Left => "yz", Item => String, Step => 3);
        package Aliased_Actuals is new Grouped_Model (Integer, 1, 1, X, X, 2, 3);
        generic
            P, Q : not null access Integer;
        procedure Bump;
        procedure Bump is
        begin
            P.all := P.all + 1;
            Q.all := Q.all + 2;
        end Bump;
        Cell : aliased Integer := 4;
        procedure Bump_Cell is new Bump (Cell'Access, Cell'Access);
        generic
            A, B : Integer;
        function Add return Integer;
        function Add return Integer is
        begin
            return A + B;
        end Add;
        function Add_One is new Add (1, 2);
        function Add_Two is new Add (B => 20, A => 10);
    begin
        if Shadow_Default.First /= 7 or Shadow_Default.Second /= 7
            or Shadow_Actual.First /= 10 or Shadow_Actual.Second /= 10
        then
            raise Program_Error;
        end if;
        if Outer_One.Value /= 46 or Outer_Two.Value /= 58 then
            raise Program_Error;
        end if;
        if Calls /= 4 or First.Sum /= 6 or Second.Sum /= 44 or Third.Sum /= 6 then
            raise Program_Error;
        end if;
        Text.Update;
        Other.Update;
        Aliased_Actuals.Update;
        Bump_Cell;
        if X /= 11 or Y /= 14 or not Text.Equal or Other.Equal
            or Cell /= 7 or Add_One /= 3 or Add_Two /= 30
        then
            raise Program_Error;
        end if;
    end Check;
    protected Results is
        procedure Passed;
        function Count return Integer;
    private
        Good : Integer := 0;
    end Results;
    protected body Results is
        procedure Passed is
        begin
            Good := Good + 1;
        end Passed;
        function Count return Integer is
        begin
            return Good;
        end Count;
    end Results;
    procedure Exercise is
        task type Worker;
        task body Worker is
        begin
            for I in 1 .. 10 loop
                Check;
            end loop;
            Results.Passed;
        end Worker;
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end Exercise;
begin
    Check;
    Exercise;
    if Results.Count /= 4 then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("grouped generic formals passed");
end GenericGrouped;
