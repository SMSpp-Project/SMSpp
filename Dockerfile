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
