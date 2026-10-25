with Ada.Text_IO;
with Operator_Visibility_Source;
with Operator_Visibility_Alias;
with Operator_Visibility_Client;
with Operator_Visibility_Parent;
procedure OperatorVisibility is
    procedure Run is
        type Number is new Integer;
        package Imported is
            function "-" (Left, Right : Number) return Number;
            function "-" (Right : Number) return Integer;
            function "+" (Right : Number) return Integer;
        end Imported;
        function "+" (Right : Number) return Integer is
        begin
            return Integer (Right) + 1;
        end "+";
        package body Imported is
            function "-" (Left, Right : Number) return Number is
            begin
                return 77;
            end "-";
            function "-" (Right : Number) return Integer is
            begin
                return -Integer (Right);
            end "-";
            function "+" (Right : Number) return Integer is
            begin
                return 999;
            end "+";
        end Imported;
        use Imported;
        procedure Check (Condition : Boolean) is
        begin
            if not Condition then raise Program_Error; end if;
        end Check;
    begin
        Check (Number'(7) - Number'(2) = 5);
        Check (-Integer'(2) = -Number'(2));
        Check (Integer'(3) = +Number'(2));
        Check (Imported."-" (Right => Number'(2), Left => Number'(7)) = 77);
        declare
            function "-" (Left, Right : Number) return Number is
            begin
                return 55;
            end "-";
        begin
            Check (Number'(7) - Number'(2) = 55);
        end;
        Check (Number'(7) - Number'(2) = 5);
        declare
            use type Operator_Visibility_Alias.Number;
            X : Operator_Visibility_Alias.Number := 2;
        begin
            Check (+X = 102 and then X ** 2 = 88);
            Check (Operator_Visibility_Alias."+" (Right => X) = 999);
            Check (Operator_Visibility_Source."=" (X, 2));
            Check (Operator_Visibility_Source."/=" (Left => X, Right => 3));
            Check (Operator_Visibility_Source."-" (5, 3) = 2);
            Check (Operator_Visibility_Source."**" (Right => 2, Left => X) = 88);
        end;
        Check (Operator_Visibility_Source."=" (Operator_Visibility_Source.Make (1), Operator_Visibility_Source.Make (1)));
        Check (Operator_Visibility_Source."/=" (Operator_Visibility_Source.Make (1), Operator_Visibility_Source.Make (2)));
        -- These literal-only calls must retain the selected operator's type.
        declare
            package Words is
                type Word is mod 256;
            end Words;
            use type Words.Word;
        begin
            Check (Words."=" (Words."+" (250, 10), 4));
            Check (Words."not" (Words.Word'(0)) = Words.Word'Last);
        end;
        Operator_Visibility_Alias.Verify;
        Operator_Visibility_Parent.Verify;
        Check (Operator_Visibility_Client);
    end Run;
    task type Worker;
    task body Worker is
    begin
        Run;
    exception
        when others => Ada.Text_IO.Put_Line ("operator worker failed");
    end Worker;
begin
    Run;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("operator visibility passed");
end OperatorVisibility;
