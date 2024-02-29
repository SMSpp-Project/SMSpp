# --------------------------------------------------------------------------- #
#    Dockerfile for CI/CD                                                     #
#                                                                             #
#    This file contains the commands to build a Docker image containing       #
#    all the packages needed to build and test the project.                   #
#    Once built and uploaded in the repository's container registry           #
#    (See: https://gitlab.com/smspp/smspp/container_registry),                #
#    the image can be fetched and used by the GitLab Runner.                  #
#                                                                             #
#    Build this with:                                                         #
#                                                                             #
#        $ docker build -t registry.gitlab.com/smspp/smspp .                  #
#                                                                             #
#    Upload with:                                                             #
#                                                                             #
#        $ docker push registry.gitlab.com/smspp/smspp                        #
#                                                                             #
#    Run (locally) with:                                                      #
#                                                                             #
#        $ docker run --rm -it registry.gitlab.com/smspp/smspp:latest         #
#                                                                             #
#    Note: you need to rebuild and upload the image only when this file       #
#          changes, not when SMS++ changes.                                   #
#                                                                             #
#                              Niccolo' Iardella                              #
#                                Donato Meoli                                 #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #

# An image containing curl and SCMs
# See: https://hub.docker.com/_/buildpack-deps
FROM buildpack-deps:scm

# Install required packages
RUN set -ex; \
		apt-get update; \
		apt-get install -y --no-install-recommends \
		clang make cmake \
		libnetcdf-c++4-dev libeigen3-dev; \
		rm -rf /var/lib/apt/lists/*; \
		update-alternatives --set cc /usr/bin/clang; \
		update-alternatives --set c++ /usr/bin/clang++;

# Install Boost
# Buster's backports repository has the 1.71.0 which is unsupported.
RUN set -ex; \
	mkdir -p "boost"; \
	curl -SL "https://dl.bintray.com/boostorg/release/1.72.0/source/boost_1_72_0.tar.gz" | \
	tar -xzC "boost" --strip-components 1; \
	cp -R "boost/boost" /usr/local/include; \
	rm -r "boost";
