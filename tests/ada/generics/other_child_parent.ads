with Generic_Family;
with Child_Support;
package Other_Child_Parent is new Generic_Family (Integer, 20, Child_Support.Increment);
