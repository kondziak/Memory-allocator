.PHONY: clean All

All:
	@echo "----------Building project:[ Allocator - Debug ]----------"
	@cd "Allocator" && "$(MAKE)" -f  "Allocator.mk"
clean:
	@echo "----------Cleaning project:[ Allocator - Debug ]----------"
	@cd "Allocator" && "$(MAKE)" -f  "Allocator.mk" clean
