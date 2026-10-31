procedure PrimitiveAuditErrors is
    not overriding procedure Local is
    begin
        null;
    end Local;
    overriding function "+" (X, Y : Integer) return Integer is
    begin
        return X - Y;
    end "+";
    package P is
        type T is tagged null record;
        function Make return T;
        procedure Defaulted (X : T := (null record));
        procedure Narrow (X : T; N : Natural);
        type Child is new T with null record;
        overriding procedure Narrow (X : Child; N : Integer);
        type Hidden is tagged private;
        overriding procedure Narrow (X : Hidden; N : Integer);
    private
        type Hidden is new T with null record;
        function Secret return Hidden;
    end P;
    package body P is
        function Make return T is
        begin
            return (null record);
        end Make;
        procedure Defaulted (X : T := (null record)) is
        begin
            null;
        end Defaulted;
        procedure Narrow (X : T; N : Natural) is
        begin
            null;
        end Narrow;
        procedure Narrow (X : Child; N : Integer) is
        begin
            null;
        end Narrow;
        procedure Narrow (X : Hidden; N : Integer) is
        begin
            null;
        end Narrow;
        function Secret return Hidden is
        begin
            return (T with null record);
        end Secret;
    end P;
begin
    null;
end PrimitiveAuditErrors;
