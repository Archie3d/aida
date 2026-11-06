procedure GenericGroupedModes is
    generic
        A, B : out Integer;
    package Bad_Mode is end Bad_Mode;
    generic
        X, Y : in out Integer := 0;
    package Bad_Default is end Bad_Default;
begin
    null;
end GenericGroupedModes;
