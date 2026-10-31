package body Array_Descriptor_Checks is
    procedure Run is
        subtype Index is Long_Integer;
        type Vector is array (Index range <>) of Integer;
        type Other is array (Index range <>) of Integer;
        type Narrow_Vector is array (Integer range <>) of Integer;
        type Matrix is array (Index range <>, Index range <>) of Integer;
        Start : Index := 5_000_000_000;
        Calls : Integer := 0;
        function Bound return Index is
        begin
            Calls := Calls + 1;
            return Start;
        end Bound;
        subtype Shifted is Other (-Start .. -Start + 2);
        A : Vector (Bound .. Start + 2) := (11, 22, 33);
        B : Other := Shifted (A);
        type Runtime_Matrix is array (Start .. Start + 1, -Start .. -Start + 2) of Integer;
        M : Matrix (Start .. Start + 1, -Start .. -Start + 2) := ((1, 2, 3), (4, 5, 6));
        R : Runtime_Matrix := ((1, 2, 3), (4, 5, 6));
        function Copy (X : Vector) return Vector is
            function Captured return Integer is
            begin
                return X (X'Last);
            end Captured;
            Y : Vector (X'First .. X'Last) := X;
        begin
            if Captured /= 33 then
                raise Program_Error;
            end if;
            return Y;
        end Copy;
        function Matrix_Copy (X : Matrix) return Matrix is
            function Captured return Integer is
            begin
                return X (X'Last (1), X'Last (2));
            end Captured;
        begin
            if Captured /= 6 then
                raise Program_Error;
            end if;
            return X;
        end Matrix_Copy;
        procedure Change (X : in out Other) is
        begin
            X (X'First) := 77;
        end Change;
        C : Vector := Copy (A);
        N : Matrix := Matrix_Copy (M);
        Low : Vector (Index'First .. Index'First + 1) := (7, 8);
        High : Vector (Index'Last - 1 .. Index'Last) := (9, 10);
        Empty : Vector (Index'Last .. Index'First);
        type Empty_Record is null record;
        type Empty_Elements is array (Index range <>) of Empty_Record;
        -- Exercise lengths above Integer'Last without allocating gigabytes.
        Huge : Empty_Elements (0 .. Start);
        function Huge_Length (X : Empty_Elements) return Long_Integer is
        begin
            return X'Length;
        end Huge_Length;
        type Static_Heap is array (Index range 5_000_000_000 .. 5_000_020_000) of Integer;
        Heap_Array : Static_Heap;
        type Holder is record
            Data : Static_Heap;
        end record;
        Local_Holder : Holder;
        function Captured_Heap return Integer is
        begin
            return Heap_Array (5_000_020_000);
        end Captured_Heap;
        function Small_Length (X : Empty_Elements) return Integer is
        begin
            return X'Length;
        end Small_Length;
        Length : Long_Integer;
        First : Index := Index'First;
        Narrow_Length : Integer;
    begin
        Heap_Array (5_000_020_000) := 12;
        Local_Holder.Data (5_000_020_000) := 17;
        if Local_Holder.Data (5_000_020_000) /= 17 then
            raise Program_Error;
        end if;
        if Captured_Heap /= 12 or C'Size /= 96 or N'Size /= 192
            or Empty'Size /= 0 or Huge'Size /= 0 then
            raise Program_Error;
        end if;
        if Calls /= 1 or A'First /= Start or A'Last /= Start + 2
            or B'First /= -Start or B (-Start + 2) /= 33
            or C /= A or C (Start + 1) /= 22
            or N /= M or N'First (2) /= -Start or R (Start + 1, -Start + 2) /= 6
            or Low (Index'First) /= 7 or High (Index'Last) /= 10
            or Empty'Length /= 0 or Empty'First /= Index'Last or Empty'Last /= Index'First
        then
            raise Program_Error;
        end if;
        begin
            declare
                Invalid_Null : Narrow_Vector := Narrow_Vector (Empty);
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
        Change (Other (A));
        if A (Start) /= 77 then
            raise Program_Error;
        end if;
        declare
            Slice : Vector renames A (Start + 1 .. Start + 2);
            Null_Slice : Vector renames A (Index'Last .. Index'First);
        begin
            if Slice'First /= Start + 1 or Slice (Start + 2) /= 33 or Null_Slice'Length /= 0 then
                raise Program_Error;
            end if;
        end;
        Length := Huge_Length (Huge);
        if Length /= Start + 1 then
            raise Program_Error;
        end if;
        declare
            Item : Empty_Record := Huge (Start);
        begin
            null;
        end;
        begin
            C (Start - 1) := 0;
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Narrow_Length := Small_Length (Huge);
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            Narrow_Length := Integer (Huge_Length (Huge));
            raise Program_Error;
        exception
            when Constraint_Error => null;
        end;
        begin
            declare
                type Too_Large is array (Index range <>) of Integer;
                X : Too_Large (First .. Index'Last);
            begin
                raise Program_Error;
            end;
        exception
            when Storage_Error => null;
        end;
    end Run;
end Array_Descriptor_Checks;
