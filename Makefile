.PHONY : cosmo app
All:
	$(MAKE) -C live555/
	$(MAKE) -C app/

app:
	$(MAKE) -C app/

live555:
	$(MAKE) -C live555/


