with Ada.Text_IO; use Ada.Text_IO;
with Ada.Finalization;
with Unchecked_Model; use Unchecked_Model;
procedure UncheckedAccess is
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then raise Program_Error; end if;
    end Check;
    function Checked (Object : access Integer) return Integer_Link is
    begin
        return Object.all'Access;
    end Checked;
    function Unchecked (Object : access Integer) return Integer_Link is
    begin
        return Object.all'Unchecked_Access;
    end Unchecked;
    generic
    package Template is
        Object : aliased Integer := 37;
        function Get return Integer_Link;
    end Template;
    package body Template is
        function Get return Integer_Link is
        begin
            return Object'Unchecked_Access;
        end Get;
    end Template;
    procedure Exercise is
        Object : aliased Integer := 11;
        Alias : Integer renames Object;
        Constant_Object : aliased constant Integer := 13;
        Holder : Component;
        type Elements is array (1 .. 3) of aliased Integer;
        Items : Elements := (2, 4, 6);
        Calls : Integer := 0;
        function Index return Integer is
        begin
            Calls := Calls + 1;
            return 2;
        end Index;
        P : Integer_Link := Alias'Unchecked_Access;
        C : Constant_Link := Constant_Object'Unchecked_Access;
        Saved : access Integer;
        type Callback is access procedure;
        function Pick return Integer_Link is
        begin
            Calls := Calls + 1;
            return P;
        end Pick;
        function Pick return Callback is
        begin
            raise Program_Error;
            return null;
        end Pick;
        procedure Capture is
        begin
            P := Object'Unchecked_Access;
        end Capture;
        function Select_Object return Integer_Link is
        begin
            Calls := Calls + 1;
            return P;
        end Select_Object;
        Text : aliased Vector := (3 .. 5 => 17);
        Grid : aliased Matrix := ((1, 2), (3, 4));
        V : Vector_Link := Text'Unchecked_Access;
        M : Matrix_Link := Grid'Unchecked_Access;
        Variant_Object : aliased Mutable;
        R : Mutable_Link := Variant_Object'Unchecked_Access;
        type Local_Child is new Root with null record;
        Local_Tagged : aliased Local_Child;
        W : Root_Link := Local_Tagged'Unchecked_Access;
        Derived : aliased Child;
        Wide : aliased Root'Class := Derived;
        Live : Integer := 0;
        type Guard is new Ada.Finalization.Controlled with null record;
        overriding procedure Initialize (Object : in out Guard);
        overriding procedure Finalize (Object : in out Guard);
        overriding procedure Initialize (Object : in out Guard) is
        begin
            Live := Live + 1;
        end Initialize;
        overriding procedure Finalize (Object : in out Guard) is
        begin
            Live := Live - 1;
        end Finalize;
        type Guard_Link is access all Guard;
        Borrowed : Guard_Link;
    begin
        Check (P.all = 11 and C.all = 13);
        P.all := 12;
        Check (Object = 12);
        P := Holder.Value'Unchecked_Access;
        P.all := 8;
        Check (Holder.Value = 8);
        P := Items (Index)'Unchecked_Access;
        Check (P.all = 4 and Calls = 1);
        P := Select_Object.all'Unchecked_Access;
        Check (P.all = 4 and Calls = 2);
        -- Context selects the object-access result, without invoking the
        -- overloaded access-to-procedure result as an implicit call.
        P := Pick.all'Unchecked_Access;
        Check (P.all = 4 and Calls = 3);
        Capture;
        Check (P.all = 12);
        Check (Unchecked (Object'Access).all = 12);
        -- Library-level accessibility must survive the anonymous parameter ABI.
        Check (Checked (Object'Unchecked_Access).all = 12);
        begin
            P := Checked (Object'Access);
            raise Constraint_Error;
        exception
            when Program_Error => Check (P.all = 12);
        end;
        declare
            Deeper : aliased Integer := 29;
            type Box (Ref : access Integer) is record
                Marker : Integer := 1;
            end record;
            View : Box (Deeper'Unchecked_Access);
            type Anonymous_Holder is record
                Ref : access Integer;
            end record;
            Part : Anonymous_Holder;
            package Instance is new Template;
        begin
            Saved := Deeper'Unchecked_Access;
            Part.Ref := Deeper'Unchecked_Access;
            Check (Saved.all = 29 and Part.Ref.all = 29);
            Check (View.Ref.all = 29 and View.Marker = 1 and Instance.Get.all = 37);
            Saved := null;
            Part.Ref := null;
        end;
        Check (V'First = 3 and V'Last = 5 and V'Length = 3);
        V (4) := 23;
        Check (Text (4) = 23 and M'Length (1) = 2 and M'Length (2) = 2);
        M (2, 1) := 31;
        Check (Grid (2, 1) = 31);
        Check (not R.all'Constrained);
        R.all := (False, Common => 19);
        Check (not Variant_Object.Present and Variant_Object.Common = 19);
        Check (W.Read = 7 and Access_Read (Wide'Unchecked_Access) = 17);
        declare
            Object : aliased Guard;
        begin
            Borrowed := Object'Unchecked_Access;
            Check (Live = 1 and Borrowed /= null);
            Borrowed := null;
        end;
        Check (Live = 0);
        P := null;
        begin
            P := P.all'Unchecked_Access;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        -- No access value is used after its designated object's scope ends.
        C := null;
        V := null;
        M := null;
        R := null;
        W := null;
    end Exercise;
    protected Results is
        procedure Done;
        function Count return Integer;
    private
        Total : Integer := 0;
    end Results;
    protected body Results is
        procedure Done is
        begin
            Total := Total + 1;
        end Done;
        function Count return Integer is
        begin
            return Total;
        end Count;
    end Results;
begin
    Exercise;
    declare
        task type Worker;
        task body Worker is
        begin
            Exercise;
            Results.Done;
        end Worker;
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Check (Results.Count = 4);
    Put_Line ("unchecked access passed");
end UncheckedAccess;
