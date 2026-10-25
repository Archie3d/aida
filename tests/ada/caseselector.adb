with Ada.Text_IO;
with Case_Selector_Model;
procedure CaseSelector is
    procedure Run is
        type Small is range 0 .. 3;
        type Byte is mod 256;
        Calls, Arguments, Branches : Natural := 0;
        function Argument return Integer is
        begin
            Arguments := Arguments + 1;
            return 2;
        end Argument;
        function Pick return Small is
        begin
            Calls := Calls + 1;
            return 2;
        end Pick;
        function Pick return Float is
        begin
            raise Program_Error;
        end Pick;
        function Pick (Value : Integer) return Small is
        begin
            Calls := Calls + 1;
            return Small (Value);
        end Pick;
        function Pick (Value : Integer) return String is
        begin
            raise Program_Error;
        end Pick;
        function Convert (Value : Integer) return Byte is
        begin
            Calls := Calls + 1;
            return Byte (Value);
        end Convert;
        function Convert (Value : Integer) return Duration is
        begin
            raise Program_Error;
        end Convert;
        function Choose (Value : Integer) return String renames Pick;
        function Cast (Value : Integer) return Duration renames Convert;
        function Flag return Boolean is
        begin
            Calls := Calls + 1;
            return True;
        end Flag;
        function Flag return Float is
        begin
            raise Program_Error;
        end Flag;
        function Letter return Character is
        begin
            Calls := Calls + 1;
            return 'A';
        end Letter;
        function Letter return String is
        begin
            raise Program_Error;
        end Letter;
        type Marker is null record;
        Object : Marker;
        function "-" (Right : Marker) return Small is
        begin
            Calls := Calls + 1;
            return 2;
        end "-";
        function "-" (Right : Marker) return Float is
        begin
            raise Program_Error;
        end "-";
        generic
            type Scalar is range <>;
            with function Choose (Value : Integer) return Scalar;
        procedure Generic_Integer;
        procedure Generic_Integer is
        begin
            case Choose (Argument) is
                when 2 => Branches := Branches + 1;
                when others => raise Program_Error;
            end case;
        end Generic_Integer;
        generic
            type Scalar is mod <>;
            with function Cast (Value : Integer) return Scalar;
        procedure Generic_Modular;
        procedure Generic_Modular is
        begin
            case Cast (Argument) is
                when 2 => Branches := Branches + 1;
                when others => raise Program_Error;
            end case;
        end Generic_Modular;
        procedure Check_Integer is new Generic_Integer (Small, Pick);
        procedure Check_Modular is new Generic_Modular (Byte, Convert);
        generic
            type Scalar is (<>);
        procedure Generic_Discrete (Value : Scalar);
        procedure Generic_Discrete (Value : Scalar) is
        begin
            case Value is
                when others => Branches := Branches + 1;
            end case;
        end Generic_Discrete;
        procedure Check_Discrete is new Generic_Discrete (Character);
        Upper : Integer := 3;
        subtype Dynamic is Integer range 1 .. Upper;
        function Dynamic_Value return Dynamic is
        begin
            Calls := Calls + 1;
            return 2;
        end Dynamic_Value;
        function Dynamic_Value return Float is
        begin
            raise Program_Error;
        end Dynamic_Value;
    begin
        case Pick is
            when 0 | 1 => raise Program_Error;
            when 2 => Branches := Branches + 1;
            when 3 => raise Program_Error;
        end case;
        case Run.Pick (Value => Argument) is
            when 2 => Branches := Branches + 1;
            when others => raise Program_Error;
        end case;
        case Convert (Argument) is
            when 0 .. 1 => raise Program_Error;
            when 2 => Branches := Branches + 1;
            when 3 .. 255 => raise Program_Error;
        end case;
        case Flag is
            when True => Branches := Branches + 1;
            when False => raise Program_Error;
        end case;
        case Letter is
            when 'A' => Branches := Branches + 1;
            when others => raise Program_Error;
        end case;
        case -Object is
            when 2 => Branches := Branches + 1;
            when others => raise Program_Error;
        end case;
        case Case_Selector_Model.Select_Value (Argument - 1) is
            when Case_Selector_Model.Middle => Branches := Branches + 1;
            when others => raise Program_Error;
        end case;
        -- Keep the selected function's nominal subtype for coverage checks.
        case Case_Selector_Model.With_Default is
            when 1 | 3 => raise Program_Error;
            when 2 => Branches := Branches + 1;
        end case;
        case Dynamic_Value is
            when 2 => Branches := Branches + 1;
            when others => raise Program_Error;
        end case;
        Check_Integer;
        Check_Modular;
        Check_Discrete ('A');
        case 1 + 1 is
            when 2 => Branches := Branches + 1;
            when others => raise Program_Error;
        end case;
        if Calls /= 9 or else Arguments /= 5 or else Branches /= 13 then
            raise Program_Error;
        end if;
    end Run;
    task type Worker;
    task body Worker is
    begin
        Run;
    exception
        when others => Ada.Text_IO.Put_Line ("case selector worker failed");
    end Worker;
begin
    Run;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("case selector resolution passed");
end CaseSelector;
