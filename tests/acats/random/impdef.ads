-- Explicit local choices for unchanged random-number tests.
package Impdef is
    Non_State_String : constant String := "invalid random state";
    Delay_Per_Random_Test : constant Duration := 1.0;
end Impdef;
