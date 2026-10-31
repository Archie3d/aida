with Ada.Text_IO;
procedure GenericPackageCallables is
    procedure Check is
        generic
            with function "+" (Left, Right : Integer) return Integer is <>;
            with function Step (Value : Integer) return Integer is Integer'Succ;
            with procedure Nothing is null;
        package Features is
            function Value return Integer;
        end Features;
        package body Features is
            function Value return Integer is
            begin
                Nothing;
                return Step (1 + 2);
            end Value;
        end Features;
        function Successor (Value : Integer) return Integer renames Integer'Succ;
        function Add (Left, Right : Integer) return Integer renames "+";
        package Data is new Features ("+" => Add, Step => Successor);
        generic
            with package F is new Features;
        function Read return Integer;
        function Read return Integer is
        begin
            return F.Value;
        end Read;
        function Read_Data is new Read (Data);
        generic
            with function Pick return Boolean;
        package Booleans is
            function Value return Boolean;
        end Booleans;
        package body Booleans is
            function Value return Boolean is
            begin
                return Pick;
            end Value;
        end Booleans;
        function True_View return Boolean renames True;
        package True_Value is new Booleans (True_View);
        generic
            with package B is new Booleans (True);
        function Read_Boolean return Boolean;
        function Read_Boolean return Boolean is
        begin
            return B.Value;
        end Read_Boolean;
        function Read_True is new Read_Boolean (True_Value);
        generic
            Text : String;
        package Labels is
            function Value return String;
        end Labels;
        package body Labels is
            function Value return String is
            begin
                return Text;
            end Value;
        end Labels;
        Word : constant String := "ab";
        package Literal_Label is new Labels ("a" & 'b');
        package Constant_Label is new Labels (Word);
        generic
            with package Label_View is new Labels ("ab");
        function Read_Label return String;
        function Read_Label return String is
        begin
            return Label_View.Value;
        end Read_Label;
        function Read_Literal is new Read_Label (Literal_Label);
        function Read_Constant is new Read_Label (Constant_Label);
    begin
        if Read_Data /= 4 or not Read_True or Read_Literal /= "ab" or Read_Constant /= "ab" then
            raise Program_Error;
        end if;
    end Check;
    task type Worker;
    task body Worker is
    begin
        Check;
    end Worker;
begin
    Check;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("formal package callable and value matching passed");
end GenericPackageCallables;
