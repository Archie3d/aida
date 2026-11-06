with System;
package Standard_Facility_Views is
    function HT_Address return System.Address;
    function Priority_Address return System.Address;
    function Null_Address return System.Address;
end Standard_Facility_Views;

package body Standard_Facility_Views is
    function HT_Address return System.Address is
    begin
        return Standard.ASCII.HT'Address;
    end HT_Address;
    function Priority_Address return System.Address is
    begin
        return System.Default_Priority'Address;
    end Priority_Address;
    function Null_Address return System.Address is
    begin
        return System.Null_Address;
    end Null_Address;
end Standard_Facility_Views;

with Standard_Facility_Views;
with Ada.Text_IO;
with System;
procedure StandardFacilities is
    use type System.Address;
    use type System.Name;
    use type System.Bit_Order;
    function Fill (Destination : System.Address; Value : Integer; Count : Long_Integer) return System.Address;
    pragma Import (C, Fill, "memset");
    package Machine renames System;
    package Codes renames Standard.ASCII;
    function Less (Left, Right : System.Address) return Boolean renames Machine."<";
    function Less_Equal (Left, Right : System.Address) return Boolean renames System."<=";
    function Greater (Left, Right : System.Address) return Boolean renames System.">";
    function Greater_Equal (Left, Right : System.Address) return Boolean renames System.">=";
    function Equal (Left, Right : System.Address) return Boolean renames System."=";
    function Unequal (Left, Right : System.Address) return Boolean renames System."/=";
    generic
        with function Compare (Left, Right : System.Address) return Boolean;
    function Apply (Left, Right : System.Address) return Boolean;
    function Apply (Left, Right : System.Address) return Boolean is
    begin
        return Compare (Left, Right);
    end Apply;
    function Generic_Less is new Apply (System."<");
    function Generic_Equal is new Apply (System."=");
    function Generic_Renamed is new Apply (Less);
    generic
        with function Compare (Left, Right : System.Address) return Boolean is System."<";
    function Default_Apply (Left, Right : System.Address) return Boolean;
    function Default_Apply (Left, Right : System.Address) return Boolean is
    begin
        return Compare (Left, Right);
    end Default_Apply;
    function Generic_Default is new Default_Apply;
    procedure Check (Value : Boolean) is
    begin
        if not Value then
            raise Program_Error;
        end if;
    end Check;
    procedure Run is
        Bytes : aliased String := "AB";
        A : constant System.Address := Bytes (1)'Address;
        B : constant System.Address := Bytes (2)'Address;
        type Address_View is new System.Address;
        function View_Less (Left, Right : Address_View) return Boolean renames "<";
        VA : Address_View := Address_View (A);
        VB : Address_View := Address_View (B);
        C : Character renames Codes.HT;
        P : System.Priority := System.Default_Priority;
        N : System.Name := System.System_Name;
        Order : System.Bit_Order := System.Default_Bit_Order;
        Zero : System.Address renames System.Null_Address;
        Highest : System.Address;
        Filled : System.Address;
        Dynamic : Integer := 31;
        Bad : Boolean := False;
        type Limits is range System.Min_Int .. System.Max_Int;
        type Real_Type is digits System.Max_Digits;
        type Fine is delta System.Fine_Delta range -1.0 .. 1.0;
        Memory : constant := System.Memory_Size;
    begin
        Check (System.Word_Size = System.Address'Size and System.Storage_Unit = 8);
        Check (Memory = 2 ** System.Word_Size);
        Check (System.Max_Base_Digits = Real_Type'Digits and System.Max_Mantissa = 63);
        Check (N = System.AIDA_64 and Order = System.Low_Order_First);
        Check (P = 15 and System.Priority'First = 0 and System.Priority'Last = 30);
        Check (System.Any_Priority'Last = 31 and System.Interrupt_Priority'First = 31);
        Filled := Fill (Highest'Address, -1, System.Address'Size / System.Storage_Unit);
        Check (Filled = Highest'Address);
        Check (Greater (Highest, A) and Less (System.Null_Address, Highest));
        Check (Generic_Less (A, Highest) and not Less (Highest, A));
        Check (Zero /= A and Zero /= B and Equal (Zero, System.Null_Address));
        Check (Less (A, B) and Less_Equal (A, A) and Greater (B, A) and Greater_Equal (B, B));
        Check (VA < VB and View_Less (VA, VB));
        Check (System.Address (VA) = A and System.Address (VB) = B);
        Check (Unequal (A, B) and not Equal (A, B));
        Check (Generic_Less (A, B) and Generic_Equal (B, B));
        Check (Generic_Renamed (A, B) and Generic_Default (A, B));
        Check (System.Name'Image (N) = "AIDA_64");
        Check (System.Bit_Order'Value ("LOW_ORDER_FIRST") = Order);
        Check (System.Default_Bit_Order'Address /= System.Null_Address);
        Check (System.System_Name'Address /= System.Null_Address);
        Check (System.Default_Priority'Address /= System.Null_Address);
        Check (System."<" (Right => B, Left => A) and A < B);
        declare
            function "<" (Left, Right : System.Address) return Boolean is
            begin
                return False;
            end "<";
            function Selected is new Apply (Machine."<");
            function Renamed (Left, Right : System.Address) return Boolean renames Machine."<";
        begin
            Check (Selected (A, B) and Renamed (A, B) and Less (A, B));
        end;
        Check (Character'Pos (Codes.NUL) = 0);
        Check (Character'Pos (Codes.SOH) = 1);
        Check (Character'Pos (Codes.STX) = 2);
        Check (Character'Pos (Codes.ETX) = 3);
        Check (Character'Pos (Codes.EOT) = 4);
        Check (Character'Pos (Codes.ENQ) = 5);
        Check (Character'Pos (Codes.ACK) = 6);
        Check (Character'Pos (Codes.BEL) = 7);
        Check (Character'Pos (Codes.BS) = 8);
        Check (Character'Pos (Codes.HT) = 9);
        Check (Character'Pos (Codes.LF) = 10);
        Check (Character'Pos (Codes.VT) = 11);
        Check (Character'Pos (Codes.FF) = 12);
        Check (Character'Pos (Codes.CR) = 13);
        Check (Character'Pos (Codes.SO) = 14);
        Check (Character'Pos (Codes.SI) = 15);
        Check (Character'Pos (Codes.DLE) = 16);
        Check (Character'Pos (Codes.DC1) = 17);
        Check (Character'Pos (Codes.DC2) = 18);
        Check (Character'Pos (Codes.DC3) = 19);
        Check (Character'Pos (Codes.DC4) = 20);
        Check (Character'Pos (Codes.NAK) = 21);
        Check (Character'Pos (Codes.SYN) = 22);
        Check (Character'Pos (Codes.ETB) = 23);
        Check (Character'Pos (Codes.CAN) = 24);
        Check (Character'Pos (Codes.EM) = 25);
        Check (Character'Pos (Codes.SUB) = 26);
        Check (Character'Pos (Codes.ESC) = 27);
        Check (Character'Pos (Codes.FS) = 28);
        Check (Character'Pos (Codes.GS) = 29);
        Check (Character'Pos (Codes.RS) = 30);
        Check (Character'Pos (Codes.US) = 31);
        Check (Character'Pos (Codes.DEL) = 127);
        Check (Character'Pos (Codes.Exclam) = 33);
        Check (Character'Pos (Codes.Quotation) = 34);
        Check (Character'Pos (Codes.Sharp) = 35);
        Check (Character'Pos (Codes.Dollar) = 36);
        Check (Character'Pos (Codes.Percent) = 37);
        Check (Character'Pos (Codes.Ampersand) = 38);
        Check (Character'Pos (Codes.Colon) = 58);
        Check (Character'Pos (Codes.Semicolon) = 59);
        Check (Character'Pos (Codes.Query) = 63);
        Check (Character'Pos (Codes.At_Sign) = 64);
        Check (Character'Pos (Codes.L_Bracket) = 91);
        Check (Character'Pos (Codes.Back_Slash) = 92);
        Check (Character'Pos (Codes.R_Bracket) = 93);
        Check (Character'Pos (Codes.Circumflex) = 94);
        Check (Character'Pos (Codes.Underline) = 95);
        Check (Character'Pos (Codes.Grave) = 96);
        Check (Character'Pos (Codes.L_Brace) = 123);
        Check (Character'Pos (Codes.Bar) = 124);
        Check (Character'Pos (Codes.R_Brace) = 125);
        Check (Character'Pos (Codes.Tilde) = 126);
        Check (Character'Pos (Codes.LC_A) = 97);
        Check (Character'Pos (Codes.LC_B) = 98);
        Check (Character'Pos (Codes.LC_C) = 99);
        Check (Character'Pos (Codes.LC_D) = 100);
        Check (Character'Pos (Codes.LC_E) = 101);
        Check (Character'Pos (Codes.LC_F) = 102);
        Check (Character'Pos (Codes.LC_G) = 103);
        Check (Character'Pos (Codes.LC_H) = 104);
        Check (Character'Pos (Codes.LC_I) = 105);
        Check (Character'Pos (Codes.LC_J) = 106);
        Check (Character'Pos (Codes.LC_K) = 107);
        Check (Character'Pos (Codes.LC_L) = 108);
        Check (Character'Pos (Codes.LC_M) = 109);
        Check (Character'Pos (Codes.LC_N) = 110);
        Check (Character'Pos (Codes.LC_O) = 111);
        Check (Character'Pos (Codes.LC_P) = 112);
        Check (Character'Pos (Codes.LC_Q) = 113);
        Check (Character'Pos (Codes.LC_R) = 114);
        Check (Character'Pos (Codes.LC_S) = 115);
        Check (Character'Pos (Codes.LC_T) = 116);
        Check (Character'Pos (Codes.LC_U) = 117);
        Check (Character'Pos (Codes.LC_V) = 118);
        Check (Character'Pos (Codes.LC_W) = 119);
        Check (Character'Pos (Codes.LC_X) = 120);
        Check (Character'Pos (Codes.LC_Y) = 121);
        Check (Character'Pos (Codes.LC_Z) = 122);
        Check (C = Character'Val (9) and ASCII.LC_Z = 'z');
        Check (Codes.Exclam = '!' and Codes.Back_Slash = Character'Val (92));
        Check (Codes.DEL = Character'Val (127) and Codes.NUL = Character'Val (0));
        Check (Codes.HT'Address /= System.Null_Address);
        Check (Codes.HT'Address = Standard_Facility_Views.HT_Address);
        Check (System.Default_Priority'Address = Standard_Facility_Views.Priority_Address);
        Check (Zero = Standard_Facility_Views.Null_Address);
        begin
            P := Dynamic;
        exception
            when Constraint_Error => Bad := True;
        end;
        Check (Bad);
    end Run;
    protected Results is
        procedure Succeed;
        function Count return Natural;
    private
        Completed : Natural := 0;
    end Results;
    protected body Results is
        procedure Succeed is
        begin
            Completed := Completed + 1;
        end Succeed;
        function Count return Natural is
        begin
            return Completed;
        end Count;
    end Results;
    task type Worker;
    task body Worker is
    begin
        Run;
        Results.Succeed;
    end Worker;
begin
    Run;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Check (Results.Count = 4);
    Ada.Text_IO.Put_Line ("standard facilities ok");
end StandardFacilities;
