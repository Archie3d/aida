with Ada.Text_IO;
with Ada.Unchecked_Conversion;
with Packed_Model; use Packed_Model;
procedure PackedStorage is
    procedure Check (N : Integer) is
        type Small is (Zero, One, Two);
        subtype Alias is Small;
        for Small'Size use 3;
        type Mapped is (Low, Middle, High);
        for Mapped'Size use 3;
        for Mapped use (Low => -3, Middle => 0, High => 3);
        type Triad is array (1 .. 3) of Boolean;
        pragma Pack (Triad);
        function To_Bits is new Ada.Unchecked_Conversion (Small, Triad);
        function From_Bits is new Ada.Unchecked_Conversion (Triad, Small);
        function From_Map is new Ada.Unchecked_Conversion (Mapped, Triad);
        function To_Map is new Ada.Unchecked_Conversion (Triad, Mapped);
        type Four is array (1 .. 4) of Boolean;
        pragma Pack (Four);
        function Mismatch is new Ada.Unchecked_Conversion (Triad, Four);
        function Resize is new Ada.Unchecked_Conversion (Bits, Eleven);
        function Again (S : Small) return Triad renames To_Bits;
        type Dynamic is array (1 .. N) of Boolean;
        pragma Pack (Dynamic);
        type Rows is array (1 .. 3) of Dynamic;
        Data : Dynamic := (others => False);
        Matrix : Rows := (others => (others => False));
        Object : Holder;
        Saved : Eleven := (others => True);
        Copy : Eleven := Object.Flags;
        Empty : Bits (7 .. 6);
        Null_Copy : Bits := not Empty;
        Calls : Integer := 0;
        function Fetch return Bits is
        begin
            Calls := Calls + 1;
            return Saved;
        end Fetch;
        Snapshot : Bits := Invert (Fetch);
        procedure Set (Value : out Boolean) is
        begin
            Value := True;
        end Set;
        procedure Fail (Value : in out Boolean) is
        begin
            Value := not Value;
            raise Constraint_Error;
        end Fail;
        procedure Set_Small (Value : out Small) is
        begin
            Value := Two;
        end Set_Small;
        type Cells is array (1 .. 3) of Small;
        Values : Cells := (Zero, One, Two);
        type Pair is record
            Left : Small := Zero;
            Right : Small := One;
        end record;
        Record_Value : Pair;
        Bad_Bits : Triad := (True, True, True);
        Raised : Boolean := False;
    begin
        if Alias'Size /= 3 or Small'Size /= 3 or Values (1)'Size < 3 or Record_Value.Left'Size < 3
            or Mapped'Size /= 3 or Triad'Size /= 3 or Data'Size < N
            or Dynamic'Size /= N or Data (1)'Size /= 1 or Object.Flags'Size < 11
        then
            raise Program_Error;
        end if;
        for Value in Small loop
            if From_Bits (Again (Value)) /= Value then raise Program_Error; end if;
        end loop;
        if To_Map (From_Map (Low)) /= Low or To_Map (From_Map (High)) /= High then
            raise Program_Error;
        end if;
        Set_Small (Values (1));
        Set_Small (Record_Value.Right);
        if Values (1) /= Two or Record_Value.Right /= Two or Small'Pos (Two) /= 2
            or Small'Image (One) /= "ONE" or Small'Succ (Zero) /= One
        then
            raise Program_Error;
        end if;
        Set (Data (1));
        Set (Data (N));
        for I in 1 .. N loop
            Matrix (2)(I) := Data (I);
            if Matrix (2)(I) /= Data (I) then raise Program_Error; end if;
        end loop;
        begin
            Fail (Data (1));
        exception
            when Constraint_Error => null;
        end;
        if not Data (1) or not Data (N) then raise Program_Error; end if;
        Object.Flags (-3) := True;
        Object.Flags (7) := True;
        Toggle (Object.Flags);
        Copy := Object.Flags;
        Saved := not Copy;
        if Copy (-3) or Copy (7) or not Copy (0) or not Saved (-3)
            or Snapshot (-3) or Calls /= 1 or Null_Copy'Length /= 0
            or (Copy xor Saved) /= Eleven'(others => True)
        then
            raise Program_Error;
        end if;
        begin
            declare
                Invalid : Small := From_Bits (Bad_Bits);
            begin
                null;
            end;
        exception
            when Constraint_Error => Raised := True;
        end;
        if not Raised then raise Program_Error; end if;
        Raised := False;
        begin
            declare
                Invalid : Four := Mismatch (To_Bits (Two));
            begin
                null;
            end;
        exception
            when Program_Error => Raised := True;
        end;
        if not Raised or Resize (Saved) /= Saved then raise Program_Error; end if;
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
                Check (I + 8);
            end loop;
            Results.Passed;
        end Worker;
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end Exercise;
begin
    Check (11);
    Exercise;
    if Results.Count /= 4 then raise Program_Error; end if;
    Ada.Text_IO.Put_Line ("packed storage passed");
end PackedStorage;
