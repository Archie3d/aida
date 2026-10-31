procedure generic_fixed_decimal is
    generic
        type Item is delta <> digits 10;
    package P is
    end P;
begin
    null;
end generic_fixed_decimal;
