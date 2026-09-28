with Ada.Text_IO; use Ada.Text_IO;
procedure PrimitiveVisibility is
    package P is
        type T is private;
        function Make (N : Integer) return T;
        function Read (X : T) return Integer;
        function "+" (X, Y : T) return T;
    private
        type T is record
            Value : Integer;
        end record;
        function Secret (X : T) return Integer;
    end P;
    package body P is
        function Make (N : Integer) return T is
        begin
            return (Value => N);
        end Make;
        function Read (X : T) return Integer is
        begin
            return X.Value;
        end Read;
        function "+" (X, Y : T) return T is
        begin
            return (Value => X.Value + Y.Value + 10);
        end "+";
        function Secret (X : T) return Integer is
        begin
            return X.Value;
        end Secret;
    end P;
    package Q is
        type T is new P.T;
        subtype Alias is T;
    end Q;
    package R_Base is
        type T is record
            Value : Integer;
        end record;
        function Make (N : Integer) return T;
        function Read (X : T) return Integer;
    end R_Base;
    package body R_Base is
        function Make (N : Integer) return T is
        begin
            return (Value => N);
        end Make;
        function Read (X : T) return Integer is
        begin
            return X.Value;
        end Read;
    end R_Base;
    package R is
        type T is private;
        overriding function Make (N : Integer) return T;
        overriding function Read (X : T) return Integer;
    private
        type T is new R_Base.T;
    end R;
    package body R is
        function Make (N : Integer) return T is
        begin
            return (Value => N);
        end Make;
        function Read (X : T) return Integer is
        begin
            return X.Value + 20;
        end Read;
    end R;
    use type Q.Alias;
    X : Q.T := Q.Make (1);
    Y : Q.T := Q.Make (2);
begin
    Put_Line (Integer'Image (Q.Read (X + Y)));
    Put_Line (Integer'Image (R.Read (R.Make (2))));
    declare
        use all type Q.T;
        function Read (X : Integer) return Integer is
        begin
            return X + 100;
        end Read;
    begin
        Put_Line (Integer'Image (Read (Make (4))));
        Put_Line (Integer'Image (Read (5)));
    end;
    declare
        package Later is
            type T is range 0 .. 10;
            use all type T;
            function Make return T;
        end Later;
        package body Later is
            function Make return T is
            begin
                return 7;
            end Make;
        end Later;
        use all type Later.T;
        X : Later.T := Make;
    begin
        Put_Line (Integer'Image (Integer (X)));
    end;
end PrimitiveVisibility;
